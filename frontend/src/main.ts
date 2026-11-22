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

const outputButton = document.getElementById(
    'test-output'
) as HTMLButtonElement;

const outputStatus = document.getElementById(
    'output-status'
) as HTMLParagraphElement;

outputButton.addEventListener('click', async () => {
    outputButton.disabled = true;
    outputStatus.textContent = 'Testing silent output…';

    try {
        await AudioService.TestOutput();
        outputStatus.textContent = 'Output test completed.';
    } catch (error) {
        outputStatus.textContent = 'Output test failed. Check the logs.';
        console.error('Native output test:', error);
    } finally {
        outputButton.disabled = false;
    }
});