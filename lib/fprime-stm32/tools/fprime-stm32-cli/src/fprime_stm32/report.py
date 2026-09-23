from dataclasses import dataclass, field
from pathlib import Path

from fprime_stm32.memory_model import MemoryMap


@dataclass
class SyncReport:
    chip_tag: str
    memory_map: MemoryMap
    linker_actions: list[str]
    startup_actions: list[str]
    linker_out: Path
    startup_out: Path
    dry_run: bool
    warnings: list[str] = field(default_factory=list)

    def render(self) -> str:
        def region_line(label: str, region) -> str:
            kib = region.length_bytes / 1024
            return f"  {label:<10} 0x{region.origin:08X}  {kib:.0f} KiB  (region {region.name!r})"

        lines = [
            f"fprime-stm32 sync - STM32H7 target: {self.chip_tag}",
            "",
            "Memory bounds extracted:",
            region_line("FLASH", self.memory_map.flash),
            region_line("AXI SRAM", self.memory_map.axi_sram),
            region_line("DTCM RAM", self.memory_map.dtcm),
            "",
            "Linker script changes:",
            *[f"  - {a}" for a in self.linker_actions],
            "",
            "Startup script changes:",
            *[f"  - {a}" for a in self.startup_actions],
            "",
        ]

        all_warnings = [*self.memory_map.warnings, *self.warnings]
        if all_warnings:
            lines.append("Warnings:")
            lines.extend(f"  ! {w}" for w in all_warnings)
            lines.append("")

        if self.dry_run:
            lines.append("(dry run - no files were written)")
            lines.append(f"  would write: {self.linker_out}")
            lines.append(f"  would write: {self.startup_out}")
        else:
            lines.append("Files written:")
            lines.append(f"  {self.linker_out}")
            lines.append(f"  {self.startup_out}")

        return "\n".join(lines)
