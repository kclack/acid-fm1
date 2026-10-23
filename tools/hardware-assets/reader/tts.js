/* SPDX-License-Identifier: MIT */
export async function createTTS() {
  if (!globalThis.speechSynthesis) throw new Error('Browser speech is unavailable.');
  return {
    unlock() {}, stop() { speechSynthesis.cancel(); },
    speak(text, options={}) {
      if (options.interrupt) speechSynthesis.cancel();
      return new Promise(resolve=>{const message=new SpeechSynthesisUtterance(text);message.onend=message.onerror=resolve;speechSynthesis.speak(message);});
    },
  };
}
