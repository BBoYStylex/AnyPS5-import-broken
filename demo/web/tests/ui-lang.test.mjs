import assert from "node:assert/strict";
import fs from "node:fs";
import path from "node:path";
import { test } from "node:test";
import { fileURLToPath } from "node:url";

const DIR = path.dirname(fileURLToPath(import.meta.url));
const elements = new Map();
const getEl = id => {
  if (!elements.has(id)) {
    elements.set(id, {
      id,
      textContent: "",
      innerHTML: "",
      hidden: false,
      disabled: false,
      value: "1",
      addEventListener: () => {},
      querySelector: () => ({ disabled: false, textContent: "" }),
      querySelectorAll: () => [],
      replaceChildren: () => {},
      classList: { add() {}, remove() {} }
    });
  }
  return elements.get(id);
};

const domElements = [
  { dataset: { i18n: "title" }, innerHTML: "PS5 apps to Windows/Linux converter" },
  { dataset: { i18n: "brand-sub" }, innerHTML: "/ Converter" },
  { dataset: { i18n: "btn-convert" }, innerHTML: "Convert &amp; save →" }
];

globalThis.window = {
  isSecureContext: true,
  Worker: class {},
  WebAssembly: {},
  ReadableStream: class {},
  WritableStream: class {},
  crypto: { subtle: {} }
};
globalThis.document = {
  documentElement: { lang: "en", dir: "ltr", dataset: {} },
  getElementById: getEl,
  querySelector: () => ({ value: "windows" }),
  querySelectorAll: selector => (selector === "[data-i18n]" ? domElements : [])
};
globalThis.localStorage = { getItem: () => null, setItem: () => {} };
globalThis.matchMedia = () => ({ matches: false });
globalThis.fetch = async () => ({ ok: false, status: 404 });

const { TRANSLATIONS, WARNING_TRANSLATIONS, ERROR_TRANSLATIONS, formatWarning, formatError, applyUiLanguage } = await import("../app.mjs");

test("covers all static data-i18n keys declared in index.html", () => {
  const html = fs.readFileSync(path.resolve(DIR, "../index.html"), "utf-8");
  const keys = [...html.matchAll(/data-i18n="([^"]+)"/g)].map(match => match[1]);
  assert(keys.length > 0);
  for (const key of keys) {
    assert(key in TRANSLATIONS.ar, `Missing translation for data-i18n key: ${key}`);
    assert.equal(typeof TRANSLATIONS.ar[key], "string");
    assert(TRANSLATIONS.ar[key].length > 0);
  }
});

test("translates dynamic archive warnings for Arabic UI", () => {
  const missingSys = "sce_sys is missing. The Windows executable will have no app icon.";
  assert.equal(formatWarning(missingSys, false), missingSys);
  assert(formatWarning(missingSys, true).includes("مجلد sce_sys مفقود"));

  const selfWarning = "test.prx is a SELF container and was skipped: unwrap it to include that module.";
  assert.equal(formatWarning(selfWarning, false), selfWarning);
  assert(formatWarning(selfWarning, true).includes("عبارة عن حاوية SELF وتم تجاوزه"));
});

test("translates runtime client errors for Arabic UI", () => {
  const batchErr = "Batch conversion accepts ZIP archives only. Choose one folder or select ZIP files.";
  assert.equal(formatError(batchErr, false), batchErr);
  assert(formatError(batchErr, true).includes("التحويل المجمع يقبل ملفات ZIP فقط"));

  const elfErr = "app.zip: Expected exactly one executable ELF in the app root; found 0";
  assert.equal(formatError(elfErr, false), elfErr);
  assert(formatError(elfErr, true).includes("لم يتم العثور على أي ملف تنفيذي"));
  assert(formatError(elfErr, true).startsWith("app.zip: "));

  const selfErr = "eboot.bin is a SELF container, not an ELF: unwrap the app before converting it";
  assert(formatError(selfErr, true).includes("عبارة عن ملف SELF مشفر"));

  const unknownErr = "Custom backend failure occurred";
  assert.equal(formatError(unknownErr, true), unknownErr);
});

test("toggles language direction and element contents between English and Arabic", () => {
  applyUiLanguage("ar");
  assert.equal(document.documentElement.lang, "ar");
  assert.equal(document.documentElement.dir, "rtl");
  assert.equal(getEl("ui-lang").textContent, "EN");
  assert(domElements[0].innerHTML.includes("محول تطبيقات PS5"));

  applyUiLanguage("en");
  assert.equal(document.documentElement.lang, "en");
  assert.equal(document.documentElement.dir, "ltr");
  assert.equal(getEl("ui-lang").textContent, "AR");
  assert.equal(domElements[0].innerHTML, "PS5 apps to Windows/Linux converter");
});
