// Fails the build when a page links to a page or an anchor that the built site does not have.
// Runs after `astro build` on dist/; links outside the site (GitHub, external) are not fetched.

import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { BASE } from '../site.config.mjs';

const dist = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', 'dist');

function htmlFiles(dir) {
  return fs.readdirSync(dir, { withFileTypes: true }).flatMap((entry) => {
    const full = path.join(dir, entry.name);
    if (entry.isDirectory()) return htmlFiles(full);
    return entry.name.endsWith('.html') ? [full] : [];
  });
}

const pages = new Map();
for (const file of htmlFiles(dist)) {
  const html = fs.readFileSync(file, 'utf8');
  const route = `${BASE}/${path.relative(dist, file).replace(/\\/g, '/').replace(/index\.html$/, '')}`;
  const ids = new Set([...html.matchAll(/\sid="([^"]+)"/g)].map((m) => m[1]));
  pages.set(route, { html, ids });
}

const problems = [];
for (const [route, { html }] of pages) {
  const content = html.split('<main')[1] ?? '';
  for (const [, href] of content.matchAll(/<a\s[^>]*href="([^"]+)"/g)) {
    if (/^(https?:|mailto:)/.test(href) && !href.includes('github.io')) continue;
    const url = new URL(href.replace(/&amp;/g, '&'), `https://site${route}`);
    if (url.host !== 'site') continue;
    const target = pages.get(url.pathname.endsWith('/') ? url.pathname : `${url.pathname}/`);
    if (!target) {
      problems.push(`${route} -> ${href} (no such page)`);
      continue;
    }
    const anchor = decodeURIComponent(url.hash.slice(1));
    if (anchor && !target.ids.has(anchor)) problems.push(`${route} -> ${href} (no such anchor)`);
  }
}

if (problems.length) {
  console.error(`check-links: ${problems.length} broken link(s)`);
  for (const problem of problems) console.error(`  ${problem}`);
  process.exit(1);
}
console.log(`check-links: ${pages.size} page(s), every internal link resolves`);
