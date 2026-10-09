/* Run the actual DOS executable in Chromium, served under the Pages subpath. */
'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const http = require('node:http');
const {chromium} = require('playwright');
const root = path.resolve(__dirname, '../build/site');
const diagnostics = path.resolve(__dirname, '../build/site-check');
const prefix = '/msdos-kakuro/';
const mime = {'.html':'text/html', '.js':'application/javascript', '.wasm':'application/wasm',
  '.css':'text/css', '.json':'application/json', '.svg':'image/svg+xml', '.png':'image/png'};
async function snapshot(page) {
  return page.evaluate(() => {
    const canvas = document.getElementById('screen');
    const data = canvas.getContext('2d').getImageData(0, 0, canvas.width, canvas.height).data;
    let hash = 2166136261, paper = 0;
    const colors = new Set();
    for (let i=0; i<data.length; i+=4) {
      const color = data[i]*65536 + data[i+1]*256 + data[i+2]; colors.add(color);
      hash = Math.imul(hash ^ color, 16777619);
      // Warm paper varies with the game's palette and DOSBox's DAC conversion.
      if (data[i]>=220 && data[i+1]>=205 && data[i+2]>=140 && data[i+2]<=190) paper++;
    }
    return {hash:hash>>>0, colors:colors.size, paper, width:canvas.width, height:canvas.height};
  });
}
async function until(page, predicate, label) {
  const deadline = Date.now() + 30000;
  let previous;
  do {
    const state = await snapshot(page);
    if (predicate(state) && state.hash === previous) return state;
    previous = predicate(state) ? state.hash : undefined;
    await page.waitForTimeout(150);
  } while (Date.now() < deadline);
  throw new Error('Timed out waiting for '+label+'; framebuffer '+JSON.stringify(await snapshot(page)));
}
async function key(page, name) {
  await page.locator('#screen').focus();
  await page.keyboard.down(name); await page.waitForTimeout(100);
  await page.keyboard.up(name);
}
async function main() {
  fs.mkdirSync(diagnostics, {recursive:true});
  const errors = [], failed = [], remote = [];
  const server = http.createServer((request, response) => {
    const url = new URL(request.url, 'http://localhost');
    if (!url.pathname.startsWith(prefix)) { response.writeHead(404); response.end(); return; }
    let file = path.resolve(root, decodeURIComponent(url.pathname.slice(prefix.length)) || 'index.html');
    if (!file.startsWith(root+path.sep)) { response.writeHead(403); response.end(); return; }
    try { const data = fs.readFileSync(file);
      response.setHeader('Content-Type', mime[path.extname(file)] || 'application/octet-stream');
      response.end(data);
    } catch (_) { response.writeHead(404); response.end('Not found'); }
  });
  await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
  let browser, page;
  try {
    browser = await chromium.launch({headless:true,
      ...(process.env.CHROMIUM_PATH ? {executablePath:process.env.CHROMIUM_PATH} : {})});
    page = await browser.newPage({viewport:{width:1200,height:1000}});
    await page.addInitScript(() => {
      window.audioStarts = 0;
      const original = AudioBufferSourceNode.prototype.start;
      AudioBufferSourceNode.prototype.start = function(...args) {
        window.audioStarts++; return original.apply(this,args);
      };
    });
    page.on('pageerror', error => errors.push(error.message));
    page.on('response', response => { if (response.status()>=400) failed.push(response.url()); });
    page.on('requestfailed', request => failed.push(request.url()));
    page.on('request', request => { if (!request.url().startsWith('http://127.0.0.1:') &&
      !/^(blob:|data:)/.test(request.url())) remote.push(request.url()); });
    const url = `http://127.0.0.1:${server.address().port}${prefix}`;
    await page.goto(url);
    await page.locator('#version').filter({hasText:'v'}).waitFor();
    await page.screenshot({path:path.join(diagnostics,'desktop.png'),fullPage:true});
    await page.click('#start');
    await page.locator('#cover').waitFor({state:'hidden',timeout:30000});
    // The four-color artwork gains a fifth color when the DOS mouse cursor shows.
    const isSplash = s=>s.width===640 && s.height===480 && s.colors>=4 && s.colors<=5;
    const splash = await until(page, isSplash, 'VGA splash');
    // The native game detects audio after drawing the splash and drains early keys.
    await page.waitForTimeout(2000);
    await key(page,'Enter');
    const journal = await until(page,s=>s.hash!==splash.hash && s.colors>=6,'puzzle journal');
    await key(page,'F1');
    await until(page,s=>s.hash!==journal.hash,'help modal');
    await key(page,'Escape');
    await until(page,s=>s.hash===journal.hash,'help dismissal');
    await key(page,'Enter');
    const board = await until(page,s=>s.paper>30000 && s.hash!==journal.hash,'puzzle board');
    await page.locator('#screen').screenshot({path:path.join(diagnostics,'game.png')});
    await key(page,'6');
    await until(page,s=>s.hash!==board.hash,'number entry');
    await key(page,'Backspace');
    await until(page,s=>s.hash===board.hash,'number erasing');
    await key(page,'ArrowRight');
    const moved = await until(page,s=>s.hash!==board.hash,'selector movement');
    await page.click('#sound');
    assert.equal(await page.locator('#sound').getAttribute('aria-pressed'),'true');
    await key(page,'ArrowLeft'); // triggers an FM navigation effect
    await page.waitForFunction(()=>window.audioStarts>0,{},{timeout:10000});
    await page.click('#pause');
    assert.equal(await page.locator('#pause').getAttribute('aria-pressed'),'true');
    const frozen = await snapshot(page);
    await key(page,'ArrowRight'); await page.waitForTimeout(200);
    assert.equal((await snapshot(page)).hash,frozen.hash,'paused board must ignore input');
    await page.click('#pause');
    const audioBeforeRestart = await page.evaluate(()=>window.audioStarts);
    await page.click('#restart');
    await until(page,s=>isSplash(s) && s.hash===splash.hash,'restart splash');
    await page.waitForTimeout(2000);
    await key(page,'Enter');
    await until(page,s=>s.colors>=6,'restarted journal');
    await page.waitForFunction(count=>window.audioStarts>count,audioBeforeRestart,{timeout:10000});
    await key(page,'Escape');
    await page.locator('#cover').waitFor({state:'visible',timeout:30000});
    assert.equal(await page.locator('#restart').isDisabled(),true);
    // Responsive page and touch controls on a fresh small viewport.
    await page.setViewportSize({width:390,height:844});
    await page.reload();
    await page.screenshot({path:path.join(diagnostics,'mobile.png'),fullPage:true});
    assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),true);
    assert.equal(await page.locator('.touch-keys').isVisible(),true);
    await page.click('#start');
    await until(page,isSplash,'mobile splash');
    await page.waitForTimeout(2000);
    await page.locator('[data-key="257"]').click();
    await until(page,s=>s.colors>=6,'touch journal');
    await page.locator('[data-key="257"]').click();
    await until(page,s=>s.paper>30000,'touch puzzle');
    const mobileBoard=await snapshot(page);
    await page.locator('[data-key="54"]').click();
    await until(page,s=>s.hash!==mobileBoard.hash,'touch number');
    assert.deepEqual(errors,[],'browser exceptions');
    assert.deepEqual(failed,[],'missing/failed resources');
    assert.deepEqual(remote,[],'all runtime resources must be self-hosted');
    assert.equal((await page.request.get(url+'KAKURO.ZIP')).status(),200);
    console.log('Browser checks passed: VGA boot, journal, entry/erase, selector, help, sound, pause, restart, exit, touch and Pages subpath.');
  } catch (error) {
    if (page) await page.screenshot({path:path.join(diagnostics,'failure.png'),fullPage:true}).catch(()=>{});
    throw error;
  } finally {
    fs.writeFileSync(path.join(diagnostics,'browser.json'),JSON.stringify({errors,failed,remote},null,2));
    if (browser) await browser.close();
    await new Promise(resolve=>server.close(resolve));
  }
}
main().catch(error=>{console.error(error);process.exitCode=1;});
