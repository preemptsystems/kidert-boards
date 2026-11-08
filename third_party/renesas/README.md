# Renesas RA device headers

Unmodified register definitions for the R7FA8M1AH (`Include/R7FA8M1AH.h` and
the `Include/system.h` it includes), from
[FSP v6.6.0](https://github.com/renesas/fsp/releases/tag/v6.6.0)
(commit `6e2675336b37fbd805f0c01d5134adfdab4bf294`), folder
`ra/fsp/src/bsp/cmsis/Device/RENESAS/Include/`. Nothing else from FSP is
used.

They are BSD-3-Clause; see [LICENSE.md](LICENSE.md), FSP's own license file.
Keep each file's copyright header when you copy or change it.

The files keep FSP's CRLF line endings, and `.gitattributes` stops git from
converting them.

## Updating

1. Download the new release from <https://github.com/renesas/fsp/releases>.
2. Copy over each file in `Include/` from the folder above, plus `LICENSE.md`
   from the release's top folder.
3. Update the version and commit above and rebuild the boards.
