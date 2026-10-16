// Values shared by astro.config.mjs and scripts/sync-docs.mjs.
// DOCS_SITE and DOCS_BASE let a fork publish under its own GitHub Pages address.

export const SITE = process.env.DOCS_SITE ?? 'https://lucaslhomme.github.io';
export const BASE = (process.env.DOCS_BASE ?? '/CrabeLoader').replace(/\/$/, '');
export const REPO_URL = 'https://github.com/LucasLhomme/CrabeLoader';
export const REPO_BRANCH = process.env.DOCS_BRANCH ?? 'dev';
