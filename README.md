# greenflame

<img src="images/greenflame_256.png" alt="Greenflame" width="128"/>

Yet another Windows screenshot tool, with very opinionated set of behaviors.  `¯\_(ツ)_/¯`

The selection scheme of [Greenshot](https://greenshot.org/)
<img src="https://avatars.githubusercontent.com/u/5614224?s=200&v=4" alt="Greenshot" style="width: 16px; height: 16px;">, the editing scheme of [Flameshot](https://flameshot.org/) <img src="https://flameshot.org/flameshot-icon.svg" alt="Flameshot" style="width: 16px; height: 16px;">, all in one tool.

**License** — [MIT](LICENSE)

---

## Usage

**Run** `greenflame.exe`. The executable sits in the tray. **Print Screen** or **right-click** the tray icon ➜  **Start capture** or **Exit**.

After **Start capture**, your screen is captured and you choose a region:

- **Click and drag** to select a rectangle.
- **Shift** + click ➜ select the whole monitor under the cursor.
- **Ctrl** + click ➜ select the window under the cursor.
- **Shift+Ctrl** + click ➜ select the whole screen.

**Drag the handles** on the selection to resize. Hold **Alt** to disable snapping.

- **Enter** or **Ctrl-S** ➜ open **Save As**, then save and close.
- **Ctrl-C** ➜ copy the selection to the clipboard, then close.
- **Escape** ➜ cancel or go back.

**Save As** supports **PNG**, **JPEG**, and **BMP**.

---

## Build

See [AGENTS.md](AGENTS.md) for toolchain and build steps.
