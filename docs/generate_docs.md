# Generate Documentation

This project uses Doxygen to generate API docs from the source code.

## Tool
- `Doxygen` (https://www.doxygen.nl)

## Install Doxygen on Windows
1. Download the installer from https://www.doxygen.nl/download.html
2. Run the installer and follow prompts
3. Confirm installation:
   ```powershell
   doxygen --version
   ```

## Generate docs (from repository root)
```powershell
cd d:\AudioDev\Projects\ResonanceEQ
doxygen Doxyfile
```

## Output location
- HTML output is generated under `docs/html`
- `docs/` is the `OUTPUT_DIRECTORY` in `Doxyfile`

## Open docs
- Open `docs/html/index.html` in your browser.
- Example:
  ```powershell
  start docs\html\index.html
  ```

## Notes
- `Doxyfile` is pre-configured for this project:
  - `PROJECT_NAME = "ResonanceEQ"`
  - `INPUT = Source`
  - `RECURSIVE = YES`
  - `GENERATE_HTML = YES`
  - `GENERATE_LATEX = NO`
  - `OUTPUT_DIRECTORY = docs`
- Ensure source comments are up-to-date when API changes.
