import re
from pathlib import Path

from fprime_stm32.linker import patch_linker_script
from fprime_stm32.memory_model import parse_memory_block

FIXTURE = Path(__file__).parent / "fixtures" / "cubemx_stm32h753" / "STM32H753xx_FLASH.ld"


def _patch():
    text = FIXTURE.read_text()
    memory_map = parse_memory_block(text)
    return patch_linker_script(text, memory_map)


def test_renames_regions():
    patched, actions = _patch()
    assert "DTCM_RAM" in patched
    assert "AXI_SRAM" in patched
    assert not re.search(r"\bDTCMRAM\b", patched)
    assert any("DTCMRAM -> DTCM_RAM" in a for a in actions)
    assert any("RAM -> AXI_SRAM" in a for a in actions)


def test_data_and_bss_retargeted_to_axi_sram():
    patched, _ = _patch()

    data_section = re.search(r"\.data\s*:\s*\{.*?\}\s*>\s*(\w+)\s*AT>\s*FLASH", patched, re.DOTALL)
    assert data_section.group(1) == "AXI_SRAM"

    bss_section = re.search(r"(?<!t)\.bss\s*\(NOLOAD\)[^{]*\{.*?\}\s*>\s*(\w+)", patched, re.DOTALL)
    assert bss_section.group(1) == "AXI_SRAM"


def test_user_heap_stack_stays_on_dtcm():
    patched, _ = _patch()
    heap_stack = re.search(r"\._user_heap_stack[^{]*\{.*?\}\s*>\s*(\w+)", patched, re.DOTALL)
    assert heap_stack.group(1) == "DTCM_RAM"


def test_dtcm_bss_section_inserted():
    patched, actions = _patch()
    dtcm_bss = re.search(r"\.dtcm_bss\s*\(NOLOAD\).*?_sdtcm_bss.*?_edtcm_bss.*?\}\s*>\s*(\w+)", patched, re.DOTALL)
    assert dtcm_bss is not None
    assert dtcm_bss.group(1) == "DTCM_RAM"
    assert any(".dtcm_bss" in a for a in actions)


def test_axi_sram_section_appended():
    patched, actions = _patch()
    axi_sram = re.search(r"\.axi_sram\s*\(NOLOAD\).*?_saxi_sram.*?_eaxi_sram.*?\}\s*>\s*(\w+)", patched, re.DOTALL)
    assert axi_sram is not None
    assert axi_sram.group(1) == "AXI_SRAM"
    assert any(".axi_sram" in a for a in actions)


def test_braces_stay_balanced():
    patched, _ = _patch()
    assert patched.count("{") == patched.count("}")
