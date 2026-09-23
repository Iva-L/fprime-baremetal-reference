import argparse
import difflib
import sys
from pathlib import Path

from fprime_stm32.discovery import discover_cubemx_sources, find_fprime_project_root
from fprime_stm32.errors import CliError
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


def _run_sync(cwd: Path, cubemx_project: Path, dry_run: bool, deployment: str | None) -> int:
    project = find_fprime_project_root(cwd, deployment=deployment)
    sources = discover_cubemx_sources(cubemx_project)

    linker_text = sources.linker_script.read_text()
    startup_text = sources.startup_script.read_text()

    memory_map = parse_memory_block(linker_text)
    patched_linker, linker_actions = patch_linker_script(linker_text, memory_map)
    patched_startup, startup_actions = patch_startup_script(startup_text)

    linker_out = project.hardware_dir / "linker" / sources.linker_script.name
    startup_out = project.hardware_dir / "startup" / sources.startup_script.name

    report = SyncReport(
        chip_tag=sources.chip_tag,
        memory_map=memory_map,
        linker_actions=linker_actions,
        startup_actions=startup_actions,
        linker_out=linker_out,
        startup_out=startup_out,
        dry_run=dry_run,
    )

    if dry_run:
        for label, before_path, after_text in (
            ("linker script", linker_out, patched_linker),
            ("startup script", startup_out, patched_startup),
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
