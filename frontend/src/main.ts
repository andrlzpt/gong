import { AudioService } from '../bindings/changeme';
import { Events } from '@wailsio/runtime';

const microphoneButton = document.getElementById(
    'test-microphone'
) as HTMLButtonElement;

const microphoneStatus = document.getElementById(
    'microphone-status'
) as HTMLParagraphElement;

function requestMicrophonePermission(): Promise<boolean> {
    const bridge = (window as Window & {
        wails?: {
            requestMicrophonePermission?: () => void;
        };
    }).wails;

    if (typeof bridge?.requestMicrophonePermission !== 'function') {
        return Promise.reject(
            new Error('Android microphone permission bridge is unavailable.')
        );
    }

    return new Promise((resolve, reject) => {
        const unsubscribe = Events.On(
            'gong:microphone-permission',
            (event) => {
                unsubscribe();
                resolve(event.data?.granted === true);
            }
        );

        try {
            bridge.requestMicrophonePermission!();
        } catch (error) {
            unsubscribe();
            reject(error);
        }
    });
}

microphoneButton.addEventListener('click', async () => {
    microphoneButton.disabled = true;
    microphoneStatus.textContent = 'Requesting access…';

    try {
        const stream = await navigator.mediaDevices.getUserMedia({
            audio: true,
        });

        stream.getTracks().forEach((track) => track.stop());
        microphoneStatus.textContent = 'Access granted. Microphone released.';
    } catch (error) {
        microphoneStatus.textContent =
            error instanceof DOMException && error.name === 'NotAllowedError'
                ? 'Microphone access denied.'
                : 'Could not access the microphone. Check the console.';

        if (error instanceof Error) {
            console.error(`Microphone: ${error.name}: ${error.message}`);
        } else {
            console.error(`Microphone: ${String(error)}`);
        }
    } finally {
        microphoneButton.disabled = false;
    }
});

const startOutputButton = document.getElementById(
    'start-output'
) as HTMLButtonElement;

const stopOutputButton = document.getElementById(
    'stop-output'
) as HTMLButtonElement;

const outputStatus = document.getElementById(
    'output-status'
) as HTMLParagraphElement;

startOutputButton.addEventListener('click', async () => {
    startOutputButton.disabled = true;
    outputStatus.textContent = 'Starting output…';

    try {
        await AudioService.StartOutput();
        outputStatus.textContent = 'Test tone running.';
        stopOutputButton.disabled = false;
    } catch (error) {
        outputStatus.textContent = 'Could not start output. Check the logs.';
        startOutputButton.disabled = false;
        stopOutputButton.disabled = false;
        console.error('Start output:', error);
    }
});

async function stopOutput() {
    startOutputButton.disabled = true;
    stopOutputButton.disabled = true;
    outputStatus.textContent = 'Stopping output…';

    try {
        await AudioService.StopOutput();
        outputStatus.textContent = 'Output stopped.';
        startOutputButton.disabled = false;
    } catch (error) {
        outputStatus.textContent = 'Could not stop output. Try again.';
        stopOutputButton.disabled = false;
        console.error('Stop output:', error);
    }
};

stopOutputButton.addEventListener('click', stopOutput);

document.addEventListener('visibilitychange', () => {
    if (document.visibilityState === 'visible') {
        void stopOutput();
        if (!inputStartPending) {
            void stopInput();
        }
    }
});

const startInputButton = document.getElementById('start-input') as HTMLButtonElement;
const stopInputButton = document.getElementById('stop-input') as HTMLButtonElement;
const inputStatus = document.getElementById('input-status') as HTMLParagraphElement;
const inputMeterState = document.getElementById('input-meter-state') as HTMLSpanElement;
const inputMeterStatus = document.getElementById('input-meter-status') as HTMLParagraphElement;

let inputStartPending = false;

const meter = document.querySelector('.input-meter') as HTMLElement;
const meterFill = document.getElementById('input-meter-fill') as unknown as SVGPathElement;
const meterValue = document.getElementById('input-meter-value') as HTMLOutputElement;
const meterClip = document.getElementById('input-meter-clip') as HTMLSpanElement;

let meterGeneration = 0;
let meterTimer: ReturnType<typeof setTimeout> | undefined;

function clearMeterDisplay() {
    meter.dataset.state = 'inactive';
    meterFill.style.strokeDashoffset = '100';
    meterValue.textContent = '— dBFS';
    meterClip.hidden = true;
}

function stopMeter() {
    meterGeneration++;
    clearTimeout(meterTimer);
    clearMeterDisplay();
}

function startMeter() {
    stopMeter();
    const generation = meterGeneration;

    async function update() {
        try {
            const peak = await AudioService.ReadInputPeak();

            if (generation !== meterGeneration) {
                return;
            }

            if (!Number.isFinite(peak) || peak < 0) {
                throw new Error('Invalid microphone peak.');
            }

            const db = 20 * Math.log10(Math.max(peak, 0.000001));
            const displayDb = Math.max(-60, Math.min(0, db));
            const percent = ((displayDb + 60) / 60) * 100;

            meterFill.style.strokeDashoffset = String(100 - percent);
            meterValue.textContent = db < -60 ? '< −60 dBFS' : `${db.toFixed(1)} dBFS`;

            const clipped = peak >= 1;
            meter.dataset.state =
                clipped ? 'clip' : db>= -6 ? 'peak' : 'active';
            meterClip.hidden = !clipped;

            inputMeterState.textContent =
                clipped ? 'CLIP' : db >= -6 ? 'PEAK' : 'LISTENING';
            inputMeterStatus.textContent = 'Native microphone input · peak level';
        } catch (error) {
            if (generation !== meterGeneration) {
                return;
            }

            clearMeterDisplay();
            inputMeterState.textContent = 'NO READING';
            inputMeterStatus.textContent = 'Waiting for a native microphone reading.';
        }

        if (generation === meterGeneration) {
            meterTimer = setTimeout(update, 50);
        }
    }

    void update();
}

startInputButton.addEventListener('click', async () => {
    if (inputStartPending) {
        return;
    }

    inputStartPending = true;
    startInputButton.disabled = true;
    microphoneButton.disabled = true;
    inputStatus.textContent = 'Starting native microphone…';

    try {
        inputStatus.textContent = 'Requesting microphone permission…';

        const granted = await requestMicrophonePermission();

        if (!granted) {
            inputStatus.textContent = 'Microphone permission denied.';
            startInputButton.disabled = false;
            microphoneButton.disabled = false;
            return;
        }

        if (document.visibilityState !== 'visible') {
            throw new Error('Return to the app before starting microphone input.');
        }

        inputStatus.textContent = 'Starting native microphone…';

        await AudioService.StartInput();
        inputStatus.textContent = 'Native microphone running.';
        startMeter();
        stopInputButton.disabled = false;
    } catch (error) {
        inputStatus.textContent = 'Could not start input. Check Android microphone permission and the logs.';
        startInputButton.disabled = false;
        stopInputButton.disabled = false;
        microphoneButton.disabled = false;
        console.error('Start input:', error);
    } finally {
        inputStartPending = false;
    }
});

async function stopInput() {
    stopMeter();
    startInputButton.disabled = true;
    stopInputButton.disabled = true;
    inputStatus.textContent = 'Stopping input…';

    try {
        await AudioService.StopInput();
        inputStatus.textContent = 'Input stopped.';
        inputMeterState.textContent = 'INACTIVE';
        inputMeterStatus.textContent = 'Microphone stopped. No level reading available.';
        startInputButton.disabled = false;
        microphoneButton.disabled = false;
    } catch (error) {
        inputStatus.textContent = 'Could not stop input. Try again.';
        stopInputButton.disabled = false;
        console.error('Stop input:', error);
    }
}

stopInputButton.addEventListener('click', stopInput);
