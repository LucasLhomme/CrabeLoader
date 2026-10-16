// Turns docs/crabe_api.def.lua (EmmyLua annotations) into one reference page per namespace.
// The definition file stays the single source of truth: IDEs read it for autocompletion,
// crabe-cli embeds it for check-api, and this module renders it for the site.

const RULE = /^-{10,}\s*$/;
const BANNER = /^-- ([A-Za-z_][\w.]*): (.+)$/;

/** Splits "type rest" where the type may hold spaces inside fun(...), quotes or after a colon. */
function splitType(text) {
  let depth = 0;
  let quote = null;
  for (let i = 0; i < text.length; i++) {
    const c = text[i];
    if (quote) {
      if (c === quote) quote = null;
      continue;
    }
    if (c === '"' || c === "'") quote = c;
    else if ('([{<'.includes(c)) depth++;
    else if (')]}>'.includes(c)) depth--;
    else if (/\s/.test(c) && depth === 0) {
      const before = text.slice(0, i).trimEnd();
      const after = text.slice(i).trimStart();
      if (before.endsWith(':') || before.endsWith('|') || after.startsWith('|')) continue;
      return [text.slice(0, i), text.slice(i).trim()];
    }
  }
  return [text.trim(), ''];
}

/** One @param line can carry several parameters ("@param x number @param y number"). */
function parseParams(text) {
  return text.split(/\s+@param\s+/).map((chunk) => {
    const [rawName, rest] = [chunk.split(/\s+/)[0], chunk.slice(chunk.split(/\s+/)[0].length).trim()];
    const [type, description] = splitType(rest);
    const optional = rawName.endsWith('?');
    return { name: rawName.replace(/\?$/, ''), optional, type, description };
  });
}

function parseReturn(text) {
  const [type, rest] = splitType(text);
  const match = rest.match(/^([a-z_][\w]*)(?:\s+(.*))?$/);
  if (match) return { type, name: match[1], description: match[2] ?? '' };
  return { type, name: '', description: rest };
}

function parseField(text) {
  const body = text.replace(/^(public|private|protected)\s+/, '');
  const name = body.split(/\s+/)[0];
  const [type, description] = splitType(body.slice(name.length).trim());
  return { name: name.replace(/\?$/, ''), optional: name.endsWith('?') || type.endsWith('?'), type, description };
}

function namespaceOf(name, current) {
  const parts = name.split('.');
  if (parts.length === 1) return current;
  if (parts[0] === 'Crabe') return parts.length === 2 ? 'Crabe' : parts.slice(0, 2).join('.');
  return parts[0];
}

export function parseLuaApi(source) {
  const lines = source.replace(/\r\n/g, '\n').split('\n');
  const namespaces = new Map();
  let current = 'Crabe';
  let doc = [];
  let tags = [];
  let afterBanner = false;
  let noteItem = null;

  const ns = (name) => {
    if (!namespaces.has(name)) namespaces.set(name, { name, title: '', notes: [], items: [], fields: [] });
    return namespaces.get(name);
  };
  const reset = () => {
    doc = [];
    tags = [];
  };
  const flushTypes = () => {
    const cls = tags.find((t) => t.tag === 'class');
    const alias = tags.find((t) => t.tag === 'alias');
    if (cls) {
      ns(current).items.push({
        kind: 'type',
        name: cls.text.split(/\s+/)[0],
        doc: doc.join(' '),
        fields: tags.filter((t) => t.tag === 'field').map((t) => parseField(t.text)),
      });
    } else if (alias) {
      const name = alias.text.split(/\s+/)[0];
      ns(current).items.push({ kind: 'alias', name, type: alias.text.slice(name.length).trim(), doc: doc.join(' ') });
    }
    reset();
  };

  for (let i = 0; i < lines.length; i++) {
    const line = lines[i];
    if (RULE.test(line)) {
      afterBanner = i > 0 && BANNER.test(lines[i - 1]) && i > 1 && RULE.test(lines[i - 2]);
      noteItem = null;
      continue;
    }
    const banner = line.match(BANNER);
    if (banner && RULE.test(lines[i - 1] ?? '') && RULE.test(lines[i + 1] ?? '')) {
      current = banner[1];
      ns(current).title = banner[2].trim();
      reset();
      continue;
    }
    if (line.startsWith('-- ') || line === '--') {
      const text = line.slice(3).trim();
      if (afterBanner) {
        ns(current).notes.push(text);
      } else if (noteItem) {
        noteItem.text += ` ${text}`;
      } else {
        noteItem = { kind: 'note', text };
        ns(current).items.push(noteItem);
      }
      continue;
    }
    afterBanner = false;
    noteItem = null;
    const tag = line.match(/^---@(\w+)\s*(.*)$/);
    if (tag) {
      if (tag[1] !== 'meta') tags.push({ tag: tag[1], text: tag[2] });
      continue;
    }
    const comment = line.match(/^---\s?(.*)$/);
    if (comment) {
      doc.push(comment[1]);
      continue;
    }
    const fn = line.match(/^function\s+([\w.:]+)\s*\(([^)]*)\)/);
    if (fn) {
      const name = fn[1].replace(':', '.');
      const documented = tags.filter((t) => t.tag === 'param').flatMap((t) => parseParams(t.text));
      const signature = fn[2].split(',').map((p) => p.trim()).filter(Boolean);
      const params = signature.map((p) => documented.find((d) => d.name === p) ?? { name: p, optional: false, type: '', description: '' });
      ns(namespaceOf(name, current)).items.push({
        kind: 'function',
        name,
        params,
        returns: tags.filter((t) => t.tag === 'return').map((t) => parseReturn(t.text)),
        doc: doc.join('\n'),
      });
      reset();
      continue;
    }
    const table = line.match(/^([\w.]+)\s*=\s*\{\s*\}/);
    if (table) {
      const name = table[1];
      const depth = name.split('.').length;
      const isNamespace = name === 'Crabe' || name === 'Game' || (name.startsWith('Crabe.') && depth === 2);
      if (isNamespace) {
        current = name;
        const fields = tags.filter((t) => t.tag === 'field').map((t) => parseField(t.text));
        ns(current).fields.push(...fields);
        if (doc.length) ns(current).notes.push(doc.join(' '));
      } else {
        ns(namespaceOf(name, current)).items.push({ kind: 'table', name, doc: doc.join(' ') });
      }
      reset();
      continue;
    }
    if (line.trim() === '') {
      if (tags.some((t) => t.tag === 'class' || t.tag === 'alias')) flushTypes();
      else reset();
    }
  }
  if (tags.length) flushTypes();
  const rank = (n) => (n.name === 'Crabe' ? 0 : n.name.startsWith('Crabe.') ? 1 : 2);
  return [...namespaces.values()]
    .filter((n) => n.items.length || n.fields.length)
    .map((n, index) => ({ n, index }))
    .sort((a, b) => rank(a.n) - rank(b.n) || a.index - b.index)
    .map(({ n }) => n);
}

export function namespaceSlug(name) {
  return `lua-api/${name.toLowerCase().replace(/\./g, '-')}`;
}

function githubSlug(text) {
  return text.toLowerCase().trim().replace(/[^\p{L}\p{N}\s_-]/gu, '').replace(/\s/g, '-');
}

const cell = (text) => (text ?? '').replace(/\|/g, '\\|').replace(/\n/g, ' ');

/** Renders every namespace as a page: { slug, title, description, body, order }. */
export function renderLuaApi(namespaces, { base, resolveText }) {
  const typeIndex = new Map();
  for (const n of namespaces) {
    for (const item of n.items) {
      if (item.kind === 'type' || item.kind === 'alias') typeIndex.set(item.name, `${base}/${namespaceSlug(n.name)}/#${githubSlug(item.name)}`);
    }
  }
  const typeRef = (type) => {
    if (!type) return '';
    const bare = type.replace(/\?$/, '').replace(/\[\]$/, '');
    const code = `\`${cell(type)}\``;
    return typeIndex.has(bare) ? `[${code}](${typeIndex.get(bare)})` : code;
  };
  const prose = (text) => resolveText(text ?? '');

  const pages = namespaces.map((n, index) => {
    const out = [];
    const lead = n.title || (n.name === 'Crabe' ? 'Core functions and version information' : '');
    if (lead) out.push(`${lead}.`, '');
    for (const note of n.notes) out.push(prose(note), '');

    if (n.fields.length) {
      out.push('## Fields', '', '| Field | Type | Description |', '| :--- | :--- | :--- |');
      for (const f of n.fields) out.push(`| \`${n.name}.${f.name}\` | ${typeRef(f.type)} | ${cell(prose(f.description))} |`);
      out.push('');
    }

    const members = n.items.filter((i) => i.kind !== 'type' && i.kind !== 'alias');
    if (members.some((i) => i.kind !== 'note')) {
      out.push('## Functions', '');
      for (const item of members) {
        if (item.kind === 'note') {
          out.push(':::note', prose(item.text), ':::', '');
          continue;
        }
        if (item.kind === 'table') {
          out.push(`### ${item.name}`, '', prose(item.doc) || 'A constant table.', '');
          continue;
        }
        const args = item.params.map((p) => (p.optional ? `${p.name}?` : p.name)).join(', ');
        const ret = item.returns.length ? ` --> ${item.returns.map((r) => r.type).join(', ')}` : '';
        out.push(`### ${item.name}`, '', '```lua', `${item.name}(${args})${ret}`, '```', '');
        if (item.doc) out.push(prose(item.doc), '');
        if (item.params.length) {
          out.push('| Parameter | Type | Description |', '| :--- | :--- | :--- |');
          for (const p of item.params) {
            const name = p.optional ? `\`${p.name}\` *(optional)*` : `\`${p.name}\``;
            out.push(`| ${name} | ${typeRef(p.type) || '—'} | ${cell(prose(p.description))} |`);
          }
          out.push('');
        }
        for (const r of item.returns) {
          const label = r.name ? ` \`${r.name}\`` : '';
          const desc = r.description ? ` — ${prose(r.description)}` : '';
          out.push(`**Returns** ${typeRef(r.type)}${label}${desc}`, '');
        }
      }
    }

    const types = n.items.filter((i) => i.kind === 'type' || i.kind === 'alias');
    if (types.length) {
      out.push('## Types', '');
      for (const t of types) {
        out.push(`### ${t.name}`, '');
        if (t.doc) out.push(prose(t.doc), '');
        if (t.kind === 'alias') {
          out.push('```lua', `---@alias ${t.name} ${t.type}`, '```', '');
          continue;
        }
        if (t.fields.length) {
          out.push('| Field | Type | Description |', '| :--- | :--- | :--- |');
          for (const f of t.fields) {
            const name = f.optional ? `\`${f.name}\` *(optional)*` : `\`${f.name}\``;
            out.push(`| ${name} | ${typeRef(f.type)} | ${cell(prose(f.description))} |`);
          }
          out.push('');
        }
      }
    }

    const functions = n.items.filter((i) => i.kind === 'function').length;
    return {
      slug: namespaceSlug(n.name),
      title: n.name,
      description: lead || `The ${n.name} Lua API.`,
      order: index + 1,
      functions,
      body: out.join('\n'),
    };
  });
  return pages;
}
