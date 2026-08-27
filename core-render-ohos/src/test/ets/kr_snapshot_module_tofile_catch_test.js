#!/usr/bin/env node
'use strict';

/*
 * Host leftover extract of KRSnapshotModule.toImage FILE / cacheKey toFile
 * catch contract. Harmony / ArkTS kits are not on host; this mirrors the
 * then/catch pairing in KRSnapshotModule.ets:
 *   FILE:     toFile reject -> callback with resultParam.message set
 *   cacheKey: callback already fired; catch logs only (no second callback,
 *             no unhandled rejection)
 */

const fs = require('fs');
const path = require('path');
const assert = require('assert');

function extractMessage(err) {
  return (err instanceof Error ? err.message : JSON.stringify(err)) ?? '';
}

function handleFileToFile(toFilePromise, resultParam, callback) {
  return toFilePromise.then((file) => {
    callback([resultParam]);
  }).catch(err => {
    resultParam.message = extractMessage(err);
    callback([resultParam]);
  });
}

function handleCacheKeyToFile(toFilePromise, resultParam, callback, log) {
  callback([resultParam]);
  return toFilePromise.catch(err => {
    log(`complete error message: ${extractMessage(err)}`);
  });
}

function waitUnhandledTick() {
  return new Promise((resolve) => setImmediate(resolve));
}

async function testFileRejectInvokesCallbackWithMessage() {
  const resultParam = { message: '' };
  const calls = [];
  await handleFileToFile(
    Promise.reject(new Error('pack failed')),
    resultParam,
    (args) => calls.push(args),
  );
  assert.strictEqual(calls.length, 1, 'FILE reject must invoke callback once');
  assert.strictEqual(resultParam.message, 'pack failed');
  assert.strictEqual(calls[0][0], resultParam);
}

async function testFileRejectNonErrorStringifies() {
  const resultParam = { message: '' };
  const calls = [];
  await handleFileToFile(
    Promise.reject({ code: 1, reason: 'open fail' }),
    resultParam,
    (args) => calls.push(args),
  );
  assert.strictEqual(calls.length, 1);
  assert.strictEqual(resultParam.message, JSON.stringify({ code: 1, reason: 'open fail' }));
}

async function testFileSuccessKeepsThenCallback() {
  const resultParam = { message: '' };
  const calls = [];
  await handleFileToFile(Promise.resolve('ok'), resultParam, (args) => calls.push(args));
  assert.strictEqual(calls.length, 1, 'FILE success must keep then(callback)');
  assert.strictEqual(resultParam.message, '', 'FILE success must not set message');
}

async function testCacheKeyRejectDoesNotDoubleCallbackOrUnhandled() {
  const resultParam = { message: '' };
  const calls = [];
  const logs = [];
  const unhandled = [];
  const onUnhandled = (reason) => { unhandled.push(reason); };
  process.on('unhandledRejection', onUnhandled);
  try {
    await handleCacheKeyToFile(
      Promise.reject(new Error('rename failed')),
      resultParam,
      (args) => calls.push(args),
      (m) => logs.push(m),
    );
    await waitUnhandledTick();
  } finally {
    process.off('unhandledRejection', onUnhandled);
  }
  assert.strictEqual(calls.length, 1, 'cacheKey reject must not call callback twice');
  assert.strictEqual(unhandled.length, 0, `cacheKey reject leaked unhandled: ${unhandled}`);
  assert.strictEqual(logs.length, 1, 'cacheKey reject must log');
  assert.ok(logs[0].includes('rename failed'));
  assert.strictEqual(resultParam.message, '', 'cacheKey reject must not overwrite message');
}

async function testCacheKeySuccessCallbackOnce() {
  const resultParam = { message: '' };
  const calls = [];
  await handleCacheKeyToFile(
    Promise.resolve('ok'),
    resultParam,
    (args) => calls.push(args),
    () => { throw new Error('cacheKey success must not log'); },
  );
  assert.strictEqual(calls.length, 1);
}

function testProductionSourceHasCatchContract() {
  const srcPath = path.join(
    __dirname,
    '../../main/ets/modules/internal/KRSnapshotModule.ets',
  );
  const src = fs.readFileSync(srcPath, 'utf8');
  const fileBlock = src.slice(src.indexOf("if (type == 'file')"), src.indexOf("if (type == 'cacheKey')"));
  const cacheBlock = src.slice(src.indexOf("if (type == 'cacheKey')"), src.indexOf('waitUntilRenderFinished'));
  assert.ok(fileBlock.includes('.then((file)'), 'FILE must keep then(success)');
  assert.ok(fileBlock.includes('.catch'), 'FILE must attach .catch');
  assert.ok(fileBlock.includes('resultParam.message'), 'FILE catch must set message');
  assert.ok(fileBlock.includes('callback(['), 'FILE catch must invoke callback');
  const cbIdx = cacheBlock.indexOf('callback([');
  const toFileIdx = cacheBlock.indexOf('KRPixelMapUtil.toFile');
  const catchIdx = cacheBlock.indexOf('.catch');
  assert.ok(cbIdx >= 0 && toFileIdx > cbIdx, 'cacheKey must callback before toFile');
  assert.ok(catchIdx > toFileIdx, 'cacheKey toFile must attach .catch');
  const catchBody = cacheBlock.slice(catchIdx);
  assert.ok(!catchBody.includes('callback('), 'cacheKey catch must not call callback again');
  assert.ok(catchBody.includes('KRRenderLog'), 'cacheKey catch must log only');
}

async function main() {
  const tests = [
    ['FILE reject invokes callback with message', testFileRejectInvokesCallbackWithMessage],
    ['FILE reject non-Error uses JSON.stringify', testFileRejectNonErrorStringifies],
    ['FILE success keeps then(callback)', testFileSuccessKeepsThenCallback],
    ['cacheKey reject no double-callback / no unhandled', testCacheKeyRejectDoesNotDoubleCallbackOrUnhandled],
    ['cacheKey success callback once', testCacheKeySuccessCallbackOnce],
    ['production source FILE/cacheKey catch contract', testProductionSourceHasCatchContract],
  ];
  let failed = 0;
  for (const [name, fn] of tests) {
    try {
      await fn();
      console.log(`PASS  ${name}`);
    } catch (e) {
      failed += 1;
      console.log(`FAIL  ${name}`);
      console.log(e && e.stack ? e.stack : e);
    }
  }
  console.log(failed ? `${failed} failed` : `${tests.length} passed`);
  process.exit(failed ? 1 : 0);
}

main();
