import argparse
import difflib
import sys
from pathlib import Path

from fprime_stm32.cubemx_cmake import parse_cubemx_cmake
from fprime_stm32.discovery import discover_cubemx_sources, find_fprime_project_root
from fprime_stm32.errors import CliError, ProjectDiscoveryError
from fprime_stm32.hardware_cmake import discover_extra_config_sources, render_hardware_cmakelists
from fprime_stm32.linker import patch_linker_script
from fprime_stm32.memory_model import parse_memory_block
from fprime_stm32.report import SyncReport
from fprime_stm32.startup import patch_startup_script


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="fprime-stm32")
    subparsers = parser.add_subparsers(dest="command", required=True)

    sync = subparsers.add_parser(
        "sync", help="Port a CubeMX STM32H7 project's linker/startup files into an F' Hardware/ directory"
    )
    sync.add_argument("cubemx_project", type=Path, help="Path to the STM32CubeMX project directory")
    sync.add_argument(
        "--dry-run", action="store_true", help="Show the proposed changes without writing any files"
    )
    sync.add_argument(
        "--deployment",
        default=None,
        help="Deployment directory name to target, when the F' project has more than one",
    )
    return parser


def _resolve_hal_dir_name(cwd: Path, cubemx_project: Path, hardware_dir: Path) -> str:
    cubemx_abs = (cubemx_project if cubemx_project.is_absolute() else cwd / cubemx_project).resolve()
    hardware_abs = hardware_dir.resolve()
    if cubemx_abs.parent != hardware_abs:
        raise ProjectDiscoveryError(
            f"{cubemx_abs} is not a direct subdirectory of {hardware_abs}. "
            "Create the CubeMX project under Hardware/<name>_hal/ (e.g. Hardware/stm32h743_hal/) "
            "and run sync against that directory."
        )
    return cubemx_abs.name


def _run_sync(cwd: Path, cubemx_project: Path, dry_run: bool, deployment: str | None) -> int:
    project = find_fprime_project_root(cwd, deployment=deployment)
    hal_dir_name = _resolve_hal_dir_name(cwd, cubemx_project, project.hardware_dir)
    sources = discover_cubemx_sources(cubemx_project)

    linker_text = sources.linker_script.read_text()
    startup_text = sources.startup_script.read_text()
    cubemx_cmake_text = sources.cubemx_cmake_file.read_text()

    linker_out = project.hardware_dir / "linker" / sources.linker_script.name
    startup_out = project.hardware_dir / "startup" / sources.startup_script.name
    cmake_out = project.hardware_dir / "CMakeLists.txt"

    existing_linker_text = linker_out.read_text() if linker_out.exists() else None

    memory_map = parse_memory_block(linker_text)
    patched_linker, linker_actions = patch_linker_script(
        linker_text, memory_map, existing_output_text=existing_linker_text
    )
    patched_startup, startup_actions = patch_startup_script(startup_text)
    cubemx_info = parse_cubemx_cmake(cubemx_cmake_text)

    extra_config_sources = discover_extra_config_sources(project.hardware_dir)
    patched_cmake = render_hardware_cmakelists(
        hal_dir_name=hal_dir_name,
        info=cubemx_info,
        linker_out_name=linker_out.name,
        startup_out_name=startup_out.name,
        extra_config_sources=extra_config_sources,
        hardware_dir=project.hardware_dir,
    )
    cmake_actions = [
        f"Regenerated Hardware/CMakeLists.txt for chip {cubemx_info.chip_define} from {hal_dir_name}/",
        f"HAL sources: {len(cubemx_info.driver_sources)} HAL driver + "
        f"{len(cubemx_info.application_sources)} peripheral-init (from CubeMX) + "
        f"{len(extra_config_sources)} project-owned (config/src/)",
        "Exported FPRIME_STM32_LINKER_SCRIPT / FPRIME_STM32_STARTUP_SOURCE / "
        "FPRIME_STM32_IT_SOURCE cache variables for the deployment CMakeLists.txt to consume",
    ]

    report = SyncReport(
        chip_tag=sources.chip_tag,
        memory_map=memory_map,
        linker_actions=linker_actions,
        startup_actions=startup_actions,
        cmake_actions=cmake_actions,
        linker_out=linker_out,
        startup_out=startup_out,
        cmake_out=cmake_out,
        dry_run=dry_run,
    )

    if dry_run:
        for label, before_path, after_text in (
            ("linker script", linker_out, patched_linker),
            ("startup script", startup_out, patched_startup),
            ("Hardware/CMakeLists.txt", cmake_out, patched_cmake),
        ):
            before_text = before_path.read_text() if before_path.exists() else ""
            diff = difflib.unified_diff(
                before_text.splitlines(keepends=True),
                after_text.splitlines(keepends=True),
                fromfile=str(before_path) if before_path.exists() else "(new file)",
                tofile=str(before_path),
            )
            diff_text = "".join(diff)
            if diff_text:
                print(f"--- diff for {label} ---")
                print(diff_text)
    else:
        linker_out.parent.mkdir(parents=True, exist_ok=True)
        startup_out.parent.mkdir(parents=True, exist_ok=True)
        linker_out.write_text(patched_linker)
        startup_out.write_text(patched_startup)
        cmake_out.write_text(patched_cmake)

    print(report.render())
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = _build_parser()
    args = parser.parse_args(argv)

    try:
        if args.command == "sync":
            return _run_sync(Path.cwd(), args.cubemx_project, args.dry_run, args.deployment)
    except CliError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    parser.error(f"unknown command: {args.command}")
    return 2


if __name__ == "__main__":
    sys.exit(main())
