// Builds src/content/docs/ from the Markdown that lives next to the code, so the repository
// stays the only place where documentation is written. Each run deletes the generated folders
// and writes them again; a page whose source is missing on this branch is skipped.
//
// CRABE_REPO_ROOT points at another checkout (default: the folder above website/).

import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { BASE, REPO_BRANCH, REPO_URL } from '../site.config.mjs';
import { parseLuaApi, renderLuaApi } from './lua-api.mjs';

const websiteDir = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const repoRoot = path.resolve(process.env.CRABE_REPO_ROOT ?? path.join(websiteDir, '..'));
const contentDir = path.join(websiteDir, 'src', 'content', 'docs');
const GENERATED_DIRS = ['start', 'guides', 'reference', 'lua-api', 'contributing'];

const NEW = { text: 'New', variant: 'note' };

const PAGES = [
  {
    slug: 'start/overview', src: 'README.md', sections: ['', 'Architecture Overview', 'Key Capabilities'],
    title: 'What is CrabeLoader?', order: 1,
    description: 'An open-source mod loader and runtime for Disney Infinity 3.0: Gold Edition (PC).',
  },
  {
    slug: 'start/installation', src: 'README.md', sections: ['Installation'],
    title: 'Installation', order: 2,
    description: 'Install CrabeLoader with CrabeInstaller.exe or by hand, update it, and remove it.',
  },
  {
    slug: 'start/folder-layout', src: 'README.md', sections: ['Modular Mod Layout'],
    title: 'Folder layout', order: 3,
    description: 'Where CrabeLoader, its configuration, logs, plugins and mods live in the game folder.',
  },
  {
    slug: 'guides/mod-development', src: 'docs/guides/mods.md',
    title: 'Mod development', order: 1,
    description: 'Manifests, lifecycle hooks, the sandbox, ImGui interfaces, memory hooks and events.',
  },
  {
    slug: 'guides/mod-basics', src: 'docs/modding.md',
    title: 'Mods and API layers', order: 2,
    description: 'Where mods live, the Crabe and Game API layers, and hot reloading with F4.',
  },
  {
    slug: 'guides/events-and-lifecycle', src: 'docs/guides/events_and_lifecycle.md',
    title: 'Events & lifecycle', order: 3, badge: NEW,
    description: 'Register mod hooks (onInit, onUpdate, onDraw, onShutdown), event bus, and quarantine.',
  },
  {
    slug: 'guides/coroutines-and-timers', src: 'docs/guides/coroutines_and_timers.md',
    title: 'Async coroutines & timers', order: 4, badge: NEW,
    description: 'Non-blocking cooperative fibers, wait delays, timers, and polling loops via Crabe.Scheduler.',
  },
  {
    slug: 'guides/storage-and-persistence', src: 'docs/guides/storage_and_persistence.md',
    title: 'Storage & persistence', order: 5, badge: NEW,
    description: 'Save and load mod data, settings, and quest states across sessions with Crabe.Storage.',
  },
  {
    slug: 'guides/drawing-and-ui', src: 'docs/guides/drawing_and_ui.md',
    title: 'Drawing primitives & UI', order: 6, badge: NEW,
    description: 'Thread-safe DrawBuffer, 2D shapes, custom HUD gauges, and Dear ImGui windows in onDraw.',
  },
  {
    slug: 'guides/characters', src: 'docs/guides/characters.md',
    title: 'Characters and figures', order: 7,
    description: 'How the game resolves a figure into a character, and how to add or expose characters.',
  },
  {
    slug: 'guides/character-creation', src: 'docs/guides/character_creation.md',
    title: 'Creating a character', order: 8,
    description: 'Build a custom character from zero, declare it in mod.json and test it in game.',
  },
  {
    slug: 'guides/virtual-reader', src: 'docs/guides/virtual_reader.md',
    title: 'Virtual reader & figures', order: 9, badge: NEW,
    description: 'Manage virtual figure slots, antenna allocation, deterministic SKUs, and in-game character swaps.',
  },
  {
    slug: 'guides/enemy-creation', src: 'docs/guides/enemy_creation.md',
    title: 'Creating enemies & bosses', order: 10, badge: NEW,
    description: 'Reskin vanilla enemy archetypes or configure custom AI, factions, stats, and Toy Box spawners.',
  },
  {
    slug: 'guides/spawners-and-weapons', src: 'docs/guides/spawners_and_weapons.md',
    title: 'Spawners & weapons', order: 11, badge: NEW,
    description: 'Spawn items, vehicles, world props, and equip lightsabers, blasters, and jetpacks from Lua.',
  },
  {
    slug: 'guides/avatar-and-combat', src: 'docs/guides/avatar_and_combat.md',
    title: 'Avatar & combat radius', order: 12, badge: NEW,
    description: 'Teleportation, coordinates, invulnerability, sparks economy, and radial area damage.',
  },
  {
    slug: 'guides/camera-and-freecam', src: 'docs/guides/camera_and_freecam.md',
    title: 'Camera & Freecam', order: 13, badge: NEW,
    description: 'Engine 6-DOF free camera, HUD suppression, editor camera modes, and cinematic photo modes.',
  },
  {
    slug: 'guides/asset-replacement', src: 'docs/guides/asset_replacement.md',
    title: 'Asset replacement', order: 14, badge: NEW,
    description: 'Replace textures, 3D meshes, materials, HUD icons, and audio with zero-touch modding.',
  },
  {
    slug: 'guides/textures', src: 'docs/guides/vfs_textures.md',
    title: 'Textures and the VFS', order: 15,
    description: 'Replace game textures with loose .tbody files served by the virtual file system.',
  },
  {
    slug: 'guides/skill-trees', src: 'docs/guides/skilltrees.md',
    title: 'Skill trees', order: 16,
    description: 'Override progression trees with Lua or patch their binary chunks.',
  },
  {
    slug: 'guides/key-bindings', src: 'docs/guides/key_bindings.md',
    title: 'Key bindings', order: 17, badge: NEW,
    description: 'The bundled keybinds mod: rebind keyboard, mouse and controller from Options > Controls.',
  },
  {
    slug: 'guides/portal', src: 'docs/guides/portal.md',
    title: 'Disney Infinity base', order: 18, badge: NEW,
    description: 'Use a physical Disney Infinity base from Lua, and give every figure its own progression.',
  },
  {
    slug: 'guides/memory-and-hooks', src: 'docs/guides/memory_and_hooks.md',
    title: 'Memory & code caves', order: 19, badge: NEW,
    description: 'AOB pattern scanning, pointer chains, safe memory reading/writing, and runtime byte patching.',
  },
  {
    slug: 'guides/asi-plugins', src: 'docs/guides/asi_plugins.md',
    title: 'Native .asi plugins', order: 20, badge: NEW,
    description: 'Load native 32-bit .asi plugins before the game starts, and talk to CrabeLoader from C.',
  },
  {
    slug: 'guides/internal-tools', src: 'docs/guides/internal_tools.md',
    title: 'CrabeLoader tool suite', order: 21, badge: NEW,
    description: 'Overview of QuickBMS, unluac decompilation, Cheat Engine bridge, and character pipeline CLI.',
  },
  {
    slug: 'guides/cli', src: 'docs/guides/cli.md',
    title: 'crabe-cli', order: 22,
    description: 'Scaffold, validate and resolve mods, and catch Lua API typos, without launching the game.',
  },
  {
    slug: 'guides/updates-and-installer', src: 'docs/guides/updates_and_installer.md',
    title: 'Updates and CrabeInstaller', order: 23,
    description: 'The launch-time update check, and what CrabeInstaller installs, refuses and rolls back.',
  },
  {
    slug: 'reference/configuration', src: 'README.md', sections: ['Configuration (`crabe.toml`)'],
    title: 'Configuration (crabe.toml)', order: 1,
    description: 'Every crabe.toml section and key, with its default.',
  },
  {
    slug: 'reference/native-db', src: 'docs/nativedb.md',
    title: 'Game natives (NativeDB)', order: 3,
    description: 'The Lua natives the game itself registers, which ones are confirmed, and how to regenerate the list.',
  },
  {
    slug: 'contributing/guide', src: 'CONTRIBUTING.md',
    title: 'Contributing', order: 1,
    description: 'Architecture invariants, development setup, Git workflow, coding standards and the PR checklist.',
  },
  {
    slug: 'contributing/building', src: 'README.md', sections: ['Building from Source'],
    title: 'Building from source', order: 2,
    description: 'Build CrabeLoader.asi, the dinput8.dll bootstrap and CrabeInstaller.exe with CMake and MSVC.',
  },
  {
    slug: 'contributing/architecture', src: 'docs/ARCHITECTURE_BLUEPRINT.md',
    title: 'Architecture blueprint', order: 3,
    description: 'The layer model, the rules every change must follow, and the definition of done.',
  },
  {
    slug: 'contributing/testing', src: 'tests/README.md',
    title: 'Tests and benchmarks', order: 4,
    description: 'The offline C++ and Lua suites, the in-game bench, and what each one covers.',
  },
  {
    slug: 'contributing/manual-checklist', src: 'docs/testing/manual_checklist.md',
    title: 'Manual test checklist', order: 5,
    description: 'The checks that need the real game, step by step.',
  },
  {
    slug: 'contributing/changelog', src: 'CHANGELOG.md',
    title: 'Changelog', order: 6,
    description: 'What changed in each CrabeLoader release.',
  },
];

const INDEX_PAGES = new Set(['docs', 'docs/README.md', 'docs/guides', 'docs/guides/README.md']);
const HTML_TAGS = new Set([
  'a', 'b', 'br', 'code', 'details', 'div', 'em', 'hr', 'i', 'img', 'kbd', 'li', 'ol', 'p', 'pre',
  'span', 'strong', 'sub', 'summary', 'sup', 'table', 'tbody', 'td', 'th', 'thead', 'tr', 'ul',
]);

function githubSlug(text) {
  return text.toLowerCase().trim().replace(/[^\p{L}\p{N}\s_-]/gu, '').replace(/\s/g, '-');
}

function read(rel) {
  const file = path.join(repoRoot, rel);
  return fs.existsSync(file) ? fs.readFileSync(file, 'utf8').replace(/\r\n/g, '\n') : null;
}

/** Calls fn(line, index) for every line outside fenced code blocks; fences pass through. */
function mapProse(text, fn) {
  let fence = null;
  return text.split('\n').map((line, index) => {
    const marker = line.match(/^\s*(`{3,}|~{3,})/);
    if (marker) {
      if (!fence) fence = marker[1];
      else if (marker[1].startsWith(fence[0]) && marker[1].length >= fence.length) fence = null;
      return line;
    }
    return fence ? line : fn(line, index);
  }).join('\n');
}

/** Applies fn to the parts of a line that are not inline code. */
function outsideInlineCode(line, fn) {
  return line.split(/(`+[^`]*`+)/).map((part, i) => (i % 2 ? part : fn(part))).join('');
}

function extractSections(markdown, names) {
  const lines = markdown.split('\n');
  const headings = [];
  let fence = false;
  lines.forEach((line, i) => {
    if (/^\s*```/.test(line)) fence = !fence;
    else if (!fence && /^#{1,2} /.test(line)) headings.push({ i, level: line.indexOf(' '), text: line.replace(/^#+ /, '').trim() });
  });
  const trim = (block) => {
    while (block.length && /^(\s*|---+)$/.test(block[0])) block.shift();
    while (block.length && /^(\s*|---+)$/.test(block[block.length - 1])) block.pop();
    return block;
  };
  const parts = names.map((name) => {
    if (name === '') {
      const h1 = headings.find((h) => h.level === 1);
      const next = headings.find((h) => h.i > (h1?.i ?? -1));
      const block = lines.slice((h1?.i ?? -1) + 1, next?.i ?? lines.length).filter((l) => !/^\[!\[/.test(l));
      const rule = block.findIndex((l) => /^---+\s*$/.test(l));
      return trim(rule >= 0 ? block.slice(0, rule) : block);
    }
    const start = headings.find((h) => h.level === 2 && h.text === name);
    if (!start) throw new Error(`README.md has no "## ${name}" section`);
    const end = headings.find((h) => h.i > start.i);
    const block = trim(lines.slice(start.i + 1, end?.i ?? lines.length));
    if (names.length === 1) return block;
    return ['', `## ${name}`, '', ...block];
  });
  let body = parts.flat().join('\n');
  if (names.length === 1) body = mapProse(body, (line) => line.replace(/^#(#{2,}) /, '$1 '));
  return body;
}

function stripTitle(markdown) {
  let done = false;
  const body = mapProse(markdown, (line) => {
    if (!done && /^# /.test(line)) {
      done = true;
      return '\u0000';
    }
    return line;
  });
  const lines = body.split('\n');
  const at = lines.indexOf('\u0000');
  const rest = at >= 0 ? lines.slice(at + 1) : lines;
  while (rest.length && /^(\s*|---+)$/.test(rest[0])) rest.shift();
  return rest.join('\n');
}

const pageBySource = new Map();
const readmeAnchors = new Map();

function indexReadme() {
  const readme = read('README.md') ?? '';
  for (const page of PAGES.filter((p) => p.src === 'README.md')) {
    for (const name of page.sections.filter(Boolean)) {
      readmeAnchors.set(githubSlug(name), { slug: page.slug, anchor: page.sections.length > 1 ? githubSlug(name) : '' });
      const start = readme.indexOf(`\n## ${name}\n`);
      const end = readme.indexOf('\n## ', start + 1);
      const block = readme.slice(start, end < 0 ? undefined : end);
      for (const m of block.matchAll(/\n#{3,} (.+)/g)) readmeAnchors.set(githubSlug(m[1]), { slug: page.slug, anchor: githubSlug(m[1]) });
    }
  }
}

function githubUrl(repoPath) {
  const full = path.join(repoRoot, repoPath);
  const kind = repoPath.endsWith('/') || (fs.existsSync(full) && fs.statSync(full).isDirectory()) ? 'tree' : 'blob';
  return `${REPO_URL}/${kind}/${REPO_BRANCH}/${repoPath.replace(/\/$/, '')}`;
}

function resolveLink(href, sourceRel) {
  if (/^(https?|mailto):/i.test(href) || href.startsWith('#')) return href;
  let target = href;
  let repoPath;
  const local = target.match(/^file:\/\/\/[a-z]:\/.*?\/CrabeLoader\/(.*)$/i);
  if (local) repoPath = local[1];
  const [rawPath, anchor = ''] = (repoPath ?? target).split('#');
  if (!repoPath) {
    if (!rawPath) return href;
    repoPath = path.posix.normalize(path.posix.join(path.posix.dirname(sourceRel), rawPath));
  } else {
    repoPath = rawPath;
  }
  repoPath = repoPath.replace(/^\.\//, '');
  if (repoPath === 'README.md') {
    const hit = readmeAnchors.get(anchor);
    if (hit) return `${BASE}/${hit.slug}/${hit.anchor ? `#${hit.anchor}` : ''}`;
    return `${BASE}/`;
  }
  if (INDEX_PAGES.has(repoPath.replace(/\/$/, ''))) return `${BASE}/`;
  if (repoPath === 'docs/crabe_api.def.lua' && !anchor) return `${BASE}/lua-api/`;
  const page = pageBySource.get(repoPath);
  if (page) return `${BASE}/${page.slug}/${anchor ? `#${anchor}` : ''}`;
  return githubUrl(repoPath) + (anchor ? `#${anchor}` : '');
}

function rewriteLinks(text, sourceRel) {
  return text.replace(/(!?)\[((?:[^\][]|\[[^\]]*\])*)\]\(([^)\s]*)(\s+"[^"]*")?\)/g, (all, bang, label, href, title = '') =>
    href ? `${bang}[${label}](${resolveLink(href, sourceRel)}${title})` : label);
}

function escapeStrayTags(text) {
  return outsideInlineCode(text, (part) =>
    part.replace(/<(\/?)([A-Za-z][^<>\n]*)>/g, (all, slash, inner) => {
      const name = inner.split(/[\s/]/)[0].toLowerCase();
      if (HTML_TAGS.has(name) || /^(https?|mailto):/i.test(inner)) return all;
      return `&lt;${slash}${inner}&gt;`;
    }));
}

function convertMermaid(text) {
  return text.replace(/^```mermaid\n([\s\S]*?)^```$/gm, (all, graph) => {
    const unlisted = graph.replace(/"(\d+)\. /g, '"​$1. ');
    const escaped = unlisted.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
    return `<pre class="mermaid">\n${escaped}</pre>`;
  });
}

/** Links bare mentions of repository Markdown files, as written in API comments. */
function linkBarePaths(text) {
  return text.replace(/(?<![\w/(\[`])((?:docs|include)\/[\w./-]+\.(?:md|h))/g, (all, rel) => `[${rel}](${resolveLink(rel, '')})`);
}

function transform(body, sourceRel) {
  const prose = mapProse(body, (line) => escapeStrayTags(rewriteLinks(line, sourceRel)));
  return convertMermaid(prose);
}

function frontmatter(page) {
  const lines = ['---', `# Generated by website/scripts/sync-docs.mjs from ${page.src}; edit that file instead.`];
  lines.push(`title: ${JSON.stringify(page.title)}`);
  if (page.description) lines.push(`description: ${JSON.stringify(page.description)}`);
  if (page.src) lines.push(`editUrl: ${JSON.stringify(`${REPO_URL}/edit/${REPO_BRANCH}/${page.src}`)}`);
  lines.push('sidebar:');
  if (page.label) lines.push(`  label: ${JSON.stringify(page.label)}`);
  lines.push(`  order: ${page.order}`);
  if (page.badge) lines.push('  badge:', `    text: ${page.badge.text}`, `    variant: ${page.badge.variant}`);
  lines.push('---', '');
  return lines.join('\n');
}

function write(page, body) {
  const file = path.join(contentDir, `${page.slug}.md`);
  fs.mkdirSync(path.dirname(file), { recursive: true });
  fs.writeFileSync(file, `${frontmatter(page)}${body.trim()}\n`);
}

function syncPages() {
  let written = 0;
  const skippedSlugs = [];
  for (const page of PAGES) {
    const source = read(page.src);
    if (source === null) {
      console.log(`  skip  ${page.slug} (${page.src} is not on this branch)`);
      skippedSlugs.push(page.slug);
      continue;
    }
    const body = page.sections ? extractSections(source, page.sections) : stripTitle(source);
    write(page, transform(body, page.src));
    written++;
  }
  fs.writeFileSync(path.join(websiteDir, '.skipped-pages.json'), JSON.stringify(skippedSlugs));
  return written;
}

function syncPluginSdk() {
  const rel = 'include/sdk/crabe_plugin.h';
  const header = read(rel);
  if (header === null) return 0;
  const guide = pageBySource.has('docs/guides/asi_plugins.md') ? `${BASE}/guides/asi-plugins/` : githubUrl('docs/guides/asi_plugins.md');
  const body = [
    `The whole C plugin API is this one header. Copy it into your plugin's sources; there is nothing to link against.`,
    `The release zip ships it under \`sdk/\`. The [native plugins guide](${guide}#talking-to-crabeloader-the-plugin-api) explains how a plugin receives the table and when each function may be called.`,
    '',
    `\`\`\`c title="${rel}"`,
    header.trimEnd(),
    '```',
  ].join('\n');
  write({
    slug: 'reference/plugin-sdk', src: rel, title: 'C plugin SDK', order: 4, badge: NEW,
    description: 'crabe_plugin.h: the table CrabeLoader hands to a native plugin, function by function.',
  }, body);
  return 1;
}

function syncLuaApi() {
  const rel = 'docs/crabe_api.def.lua';
  const source = read(rel);
  if (source === null) return 0;
  const namespaces = parseLuaApi(source);
  const resolveText = (text) => escapeStrayTags(linkBarePaths(text));
  const pages = renderLuaApi(namespaces, { base: BASE, resolveText });
  for (const page of pages) write({ ...page, src: rel }, page.body);

  const rows = pages.map((p) => `| [\`${p.title}\`](${BASE}/${p.slug}/) | ${p.description} | ${p.functions} |`);
  const overview = [
    'Every function a mod can call, generated from [`docs/crabe_api.def.lua`](' + githubUrl(rel) + '), the same file your editor reads for autocompletion.',
    '',
    ':::tip[Autocompletion in your editor]',
    'Copy `crabe_api.def.lua` into your mod workspace (it also ships in the release zip under `docs/`). The Lua Language Server picks it up and completes every `Crabe.*` and `Game.*` call, with these descriptions.',
    ':::',
    '',
    '## Namespaces',
    '',
    '| Namespace | What it covers | Functions |',
    '| :--- | :--- | ---: |',
    ...rows,
    '',
    '## Two layers',
    '',
    '- **`Crabe.*`** is CrabeLoader\'s own, stable API: the mod lifecycle, events, input, ImGui drawing, menus, settings, the VFS, plugins and characters. It is the same on every supported game build.',
    '- **`Game.*`** is a guarded facade over the game\'s own Lua natives. It checks its arguments and the game state before calling into the engine. The raw natives are listed in [Game natives (NativeDB)](' + `${BASE}/reference/native-db/` + ').',
  ].join('\n');
  write({
    slug: 'lua-api/index', src: rel, title: 'Lua API reference', label: 'Overview', order: 0,
    description: 'Every Crabe.* and Game.* function a Lua mod can call, with parameters, returns and types.',
  }, overview);
  return pages.length + 1;
}

for (const dir of GENERATED_DIRS) fs.rmSync(path.join(contentDir, dir), { recursive: true, force: true });
for (const page of PAGES) if (read(page.src) !== null && !page.sections) pageBySource.set(page.src, page);
if (read('include/sdk/crabe_plugin.h') !== null) pageBySource.set('include/sdk/crabe_plugin.h', { slug: 'reference/plugin-sdk' });
indexReadme();

const total = syncPages() + syncPluginSdk() + syncLuaApi();
console.log(`sync-docs: ${total} page(s) generated from ${repoRoot}`);
