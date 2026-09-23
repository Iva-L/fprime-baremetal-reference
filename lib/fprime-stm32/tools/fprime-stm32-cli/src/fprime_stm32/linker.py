import re

from fprime_stm32.errors import LinkerScriptError
from fprime_stm32.memory_model import MemoryMap

DTCM_BSS_BLOCK = """.dtcm_bss (NOLOAD) : ALIGN(32)
{
  . = ALIGN(32);
  _sdtcm_bss = .;

  *(.dtcm_bss)
  *(.dtcm_bss.*)

  . = ALIGN(32);
  _edtcm_bss = .;
} >DTCM_RAM

"""

AXI_SRAM_BLOCK = """.axi_sram (NOLOAD) : ALIGN(32)
{
  . = ALIGN(32);
  _saxi_sram = .;

  *(.axi_sram)
  *(.axi_sram.*)

  . = ALIGN(32);
  _eaxi_sram = .;
} >AXI_SRAM

"""

_DATA_TDATA_RETARGET_RE = re.compile(
    r"^([ \t]*\.(?:data|tdata)\b.*?\{.*?\})\s*>\s*DTCM_RAM\s*AT>\s*FLASH",
    re.DOTALL | re.MULTILINE,
)
_TBSS_BSS_RETARGET_RE = re.compile(
    r"^([ \t]*\.(?:tbss|bss)\b.*?\{.*?\})\s*>\s*DTCM_RAM\b(?!\s*AT>)",
    re.DOTALL | re.MULTILINE,
)
_PLAIN_BSS_HEADER_RE = re.compile(r"^([ \t]*)\.bss\b", re.MULTILINE)
_DISCARD_RE = re.compile(r"^[ \t]*/DISCARD/", re.MULTILINE)


def _rename_region(text: str, old_name: str, new_name: str) -> str:
    """Rename a MEMORY region, touching only linker-script syntax (never comments/prose):
    the MEMORY{} table entry, ORIGIN()/LENGTH() calls, and '>NAME' output-section selectors.
    """
    escaped = re.escape(old_name)
    text = re.sub(rf"^(?P<indent>[ \t]*){escaped}(?=[ \t]*\()", rf"\g<indent>{new_name}", text, flags=re.MULTILINE)
    text = re.sub(rf"\b(ORIGIN|LENGTH)\(\s*{escaped}\s*\)", rf"\1({new_name})", text)
    text = re.sub(rf">(\s*){escaped}\b", rf">\1{new_name}", text)
    return text


def patch_linker_script(text: str, memory_map: MemoryMap) -> tuple[str, list[str]]:
    actions: list[str] = []

    dtcm_old = memory_map.dtcm.name
    axi_old = memory_map.axi_sram.name

    patched = text
    if dtcm_old != "DTCM_RAM":
        patched = _rename_region(patched, dtcm_old, "DTCM_RAM")
        actions.append(f"Renamed memory region {dtcm_old} -> DTCM_RAM")
    if axi_old != "AXI_SRAM":
        patched = _rename_region(patched, axi_old, "AXI_SRAM")
        actions.append(f"Renamed memory region {axi_old} -> AXI_SRAM")

    patched, n = _DATA_TDATA_RETARGET_RE.subn(r"\1 >AXI_SRAM AT> FLASH", patched)
    if n == 0:
        raise LinkerScriptError(
            "Could not find the .data/.tdata output sections targeting DTCM RAM "
            "(unrecognized linker script template) - refusing to patch"
        )
    actions.append(f"Retargeted {n} initialized-data section(s) (.data/.tdata) to AXI_SRAM")

    patched, n = _TBSS_BSS_RETARGET_RE.subn(r"\1 >AXI_SRAM", patched)
    if n == 0:
        raise LinkerScriptError(
            "Could not find the .tbss/.bss output sections targeting DTCM RAM "
            "(unrecognized linker script template) - refusing to patch"
        )
    actions.append(f"Retargeted {n} zero-initialized section(s) (.tbss/.bss) to AXI_SRAM")

    header_match = _PLAIN_BSS_HEADER_RE.search(patched)
    if header_match is None:
        raise LinkerScriptError("Could not find the .bss output section to anchor .dtcm_bss insertion")
    indent = header_match.group(1)
    dtcm_block = "\n".join(
        (indent + line) if line else line for line in DTCM_BSS_BLOCK.rstrip("\n").split("\n")
    ) + "\n\n"
    patched = patched[: header_match.start()] + dtcm_block + patched[header_match.start() :]
    actions.append("Inserted .dtcm_bss NOLOAD section (DTCM_RAM) with _sdtcm_bss/_edtcm_bss")

    axi_block = "\n".join(
        (indent + line) if line else line for line in AXI_SRAM_BLOCK.rstrip("\n").split("\n")
    ) + "\n\n"
    discard_match = _DISCARD_RE.search(patched)
    if discard_match is not None:
        insert_at = patched.rfind("\n", 0, discard_match.start()) + 1
        patched = patched[:insert_at] + axi_block + patched[insert_at:]
    else:
        last_brace = patched.rstrip().rfind("}")
        patched = patched[:last_brace] + axi_block + patched[last_brace:]
    actions.append("Appended .axi_sram NOLOAD section (AXI_SRAM) with _saxi_sram/_eaxi_sram")

    return patched, actions
