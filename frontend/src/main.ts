import { AudioService } from '../bindings/changeme';

const microphoneButton = document.getElementById(
    'test-microphone'
) as HTMLButtonElement;

const microphoneStatus = document.getElementById(
    'microphone-status'
) as HTMLParagraphElement;

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
        outputStatus.textContent = 'Silent output running.';
        stopOutputButton.disabled = false;
    } catch (error) {
        outputStatus.textContent = 'Could not start output. Check the logs.';
        startOutputButton.disabled = false;
        stopOutputButton.disabled = false;
        console.error('Start output:', error);
    }
});

stopOutputButton.addEventListener('click', async () => {
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
});