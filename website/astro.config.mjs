// The CrabeLoader documentation site: Starlight over the Markdown that lives in the repository.
// scripts/sync-docs.mjs writes src/content/docs/ before every dev server or build.

import { defineConfig } from 'astro/config';
import starlight from '@astrojs/starlight';
import { BASE, REPO_BRANCH, REPO_URL, SITE } from './site.config.mjs';

const mermaid = `
const blocks = document.querySelectorAll('pre.mermaid');
if (blocks.length) {
  const { default: mermaid } = await import('https://cdn.jsdelivr.net/npm/mermaid@11.4.1/dist/mermaid.esm.min.mjs');
  mermaid.initialize({ startOnLoad: false, theme: 'neutral', fontFamily: 'inherit' });
  await mermaid.run({ nodes: blocks });
}`;

export default defineConfig({
  site: SITE,
  base: BASE,
  trailingSlash: 'always',
  integrations: [
    starlight({
      title: 'CrabeLoader',
      description: 'The open-source mod loader and runtime for Disney Infinity 3.0: Gold Edition (PC).',
      logo: { src: './src/assets/crab.svg', alt: 'CrabeLoader' },
      favicon: '/favicon.svg',
      social: [{ icon: 'github', label: 'GitHub', href: REPO_URL }],
      editLink: { baseUrl: `${REPO_URL}/edit/${REPO_BRANCH}/website/` },
      customCss: ['./src/styles/theme.css'],
      head: [{ tag: 'script', attrs: { type: 'module' }, content: mermaid }],
      expressiveCode: { themes: ['github-dark-dimmed', 'github-light'] },
      sidebar: [
        { label: 'Start here', items: [{ autogenerate: { directory: 'start' } }] },
        { label: 'Modding guides', items: [{ autogenerate: { directory: 'guides' } }] },
        {
          label: 'Reference',
          items: [
            { autogenerate: { directory: 'reference' } },
            { label: 'Lua API', collapsed: true, items: [{ autogenerate: { directory: 'lua-api' } }] },
          ],
        },
        { label: 'Contributing', collapsed: true, items: [{ autogenerate: { directory: 'contributing' } }] },
      ],
    }),
  ],
});
