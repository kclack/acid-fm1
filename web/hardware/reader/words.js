/* SPDX-License-Identifier: MIT */
export const wordsOf = data => Array.isArray(data.words) ? data.words : [];
export function spoken(text, words) {
  for (const [source, replacement] of [...words].sort((a,b)=>b[0].length-a[0].length)) {
    text=text.split(source).join(replacement);
  }
  return text;
}
