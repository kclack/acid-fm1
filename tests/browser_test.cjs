/* SPDX-License-Identifier: MIT
 * Run with PLAYWRIGHT_MODULE pointing to an installed Playwright module.
 * BROWSER_BIN defaults to /usr/bin/chromium; BASE_URL defaults to localhost:8000.
 */
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const {chromium}=require(process.env.PLAYWRIGHT_MODULE||'playwright');
(async()=>{
  const out=path.resolve('build/browser-test');fs.mkdirSync(out,{recursive:true});
  const browser=await chromium.launch({executablePath:process.env.BROWSER_BIN||'/usr/bin/chromium',headless:true,args:['--no-sandbox','--autoplay-policy=no-user-gesture-required']});
  const page=await browser.newPage({viewport:{width:1440,height:1050},acceptDownloads:true}),errors=[],failures=[],requests=[];
  page.on('pageerror',e=>errors.push(e.message));page.on('response',r=>{if(r.status()>=400)failures.push(`${r.status()} ${r.url()}`);});page.on('request',r=>requests.push(r.url()));
  await page.goto(process.env.BASE_URL||'http://127.0.0.1:8000/');await page.waitForFunction(()=>window.acidLab);
  assert.equal(await page.evaluate(()=>acidLab.project.params.length),53);
  await page.click('#play');await page.waitForFunction(()=>acidLab.context?.state==='running');await page.waitForTimeout(500);
  assert(await page.evaluate(()=>acidLab.playing));
  const level=await page.evaluate(()=>{const x=new Float32Array(1024);acidLab.analyser.getFloatTimeDomainData(x);return Math.max(...x.map(Math.abs));});assert(level>.005,'live AudioWorklet emits sound');
  await page.locator('[data-param="0"]').press('ArrowUp');
  const patchBefore=await page.evaluate(()=>acidLab.project);await page.click('#save');await page.click('#generate');const generated=await page.evaluate(()=>acidLab.project);assert.notDeepEqual(generated.steps,patchBefore.steps);
  await page.click('#undo');assert.deepEqual((await page.evaluate(()=>acidLab.project)).steps,patchBefore.steps);
  await page.click('#load');assert.deepEqual((await page.evaluate(()=>acidLab.project)).params,patchBefore.params);
  await page.click('[data-bank="3"]');await page.fill('[aria-label="Step 49 note offset"]','19');await page.locator('[aria-label="Step 49 note offset"]').press('Tab');assert.equal((await page.evaluate(()=>acidLab.project)).steps[48][0],19);
  await page.click('[data-tab="modulation"]');await page.locator('[data-param="34"]').selectOption('1');await page.locator('#curve-0-0').focus();await page.keyboard.press('ArrowUp');
  assert.equal((await page.evaluate(()=>acidLab.project)).params[34],1);
  await page.screenshot({path:path.join(out,'modulation.png'),fullPage:true});
  await page.click('[data-tab="sequence"]');await page.click('#panic');assert(!(await page.evaluate(()=>acidLab.playing)));
  for(const [id,name]of [['export-project','patch.json'],['midi-export','pattern.mid'],['wav-export','pattern.wav']]){
    const pending=page.waitForEvent('download');await page.click('#'+id);const d=await pending;await d.saveAs(path.join(out,name));
  }
  const exported=JSON.parse(fs.readFileSync(path.join(out,'patch.json')));assert.equal(exported.format,'acid-fm1');
  assert.equal(fs.readFileSync(path.join(out,'pattern.mid')).subarray(0,4).toString(),'MThd');
  const wav=fs.readFileSync(path.join(out,'pattern.wav'));assert.equal(wav.subarray(0,4).toString(),'RIFF');assert(wav.length>100000);
  await page.click('#generate');await page.setInputFiles('#project-file',path.join(out,'patch.json'));await page.waitForTimeout(100);assert.deepEqual((await page.evaluate(()=>acidLab.project)).steps,exported.steps);
  await page.screenshot({path:path.join(out,'desktop.png'),fullPage:true});
  await page.setViewportSize({width:390,height:844});assert(!(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth)));
  await page.screenshot({path:path.join(out,'mobile.png'),fullPage:true});
  await page.goto((process.env.BASE_URL||'http://127.0.0.1:8000/')+'guide.html');assert(await page.locator('h1').isVisible());
  await page.goto((process.env.BASE_URL||'http://127.0.0.1:8000/')+'hardware/');
  await page.locator('#go').click();await page.waitForTimeout(700);await page.screenshot({path:path.join(out,'hardware.png'),fullPage:true});
  assert(!((await page.locator('#status').textContent())||'').match(/fail|error/i));
  await browser.close();
  assert.deepEqual(errors,[]);assert.deepEqual(failures,[]);
  const thirdParty=requests.filter(u=>!u.startsWith(process.env.BASE_URL||'http://127.0.0.1:8000/')&&!u.startsWith('blob:')&&!u.startsWith('data:'));
  assert.deepEqual(thirdParty,[],'no external resources loaded by the emulator');
  console.log(JSON.stringify({pass:true,livePeak:level,downloads:['patch.json','pattern.mid','pattern.wav'],externalRequests:0,artifacts:out}));
})().catch(e=>{console.error(e);process.exit(1);});
