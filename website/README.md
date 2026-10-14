# CrabeLoader documentation site

The site published at <https://lucaslhomme.github.io/CrabeLoader/>, built with [Starlight](https://starlight.astro.build/).

**Write documentation where it already lives, not here.** The pages are generated from the repository's own Markdown at every build, so a guide stays next to the code it describes and still reads well on GitHub.

| Site section | Source |
| :--- | :--- |
| Start here | Sections of the root `README.md` |
| Modding guides | `docs/guides/*.md`, `docs/modding.md` |
| Reference | `README.md` (configuration), `docs/nativedb.md`, `include/sdk/crabe_plugin.h` |
| Lua API | `docs/crabe_api.def.lua`, one page per namespace |
| Contributing | `CONTRIBUTING.md`, `README.md` (building), `docs/ARCHITECTURE_BLUEPRINT.md`, `tests/README.md`, `docs/testing/manual_checklist.md`, `CHANGELOG.md` |

Only the landing page (`src/content/docs/index.mdx`), the theme and the site configuration are written in this folder.

## Run it locally

Node.js 22.12 or later:

```powershell
cd website
npm install
npm run dev      # http://localhost:4321/CrabeLoader/, rebuilt as you save
npm run build    # the full site in dist/, then the link check
```

`npm run dev` copies the Markdown once when it starts; restart it after editing a file outside `website/`.

## How a source file becomes a page

`scripts/sync-docs.mjs` lists every page: its source file, its title, its place in the sidebar and, for the root `README.md`, which `##` sections it takes. For each one it:

- drops the source's `# Title` (the page title comes from the list) and writes the page to `src/content/docs/`;
- turns links between Markdown files into links between pages, anchors included, and links to any other repository file into a GitHub link;
- renders ` ```mermaid ` blocks as diagrams;
- sets the page's *Edit page* link to the source file on GitHub.

`scripts/lua-api.mjs` reads the EmmyLua annotations of `docs/crabe_api.def.lua` and writes one page per namespace: functions with their parameters and returns, then types, with every known type linked to its definition.

To add a guide, write it in `docs/guides/` and add one entry to `PAGES` in `scripts/sync-docs.mjs`. A page whose source is missing on the current branch is skipped, so the site builds on every branch.

## Publishing

`.github/workflows/docs.yml` builds the site on every pull request that touches documentation (a broken link fails it) and publishes it to GitHub Pages on every push to `main`, so the site always matches the latest release. It can also be started by hand from the Actions tab.

GitHub Pages must be set once to **Settings → Pages → Source: GitHub Actions**.
