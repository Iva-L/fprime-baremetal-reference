import shutil
from pathlib import Path

import pytest

from fprime_stm32.cli import main

FIXTURE_DIR = Path(__file__).parent / "fixtures" / "cubemx_stm32h753"


@pytest.fixture
def cubemx_project(tmp_path):
    dest = tmp_path / "cubemx_project"
    shutil.copytree(FIXTURE_DIR, dest)
    return dest


@pytest.fixture
def fprime_project(tmp_path, monkeypatch):
    deployment = tmp_path / "MyDeployment"
    (deployment / "Hardware").mkdir(parents=True)
    monkeypatch.chdir(tmp_path / "MyDeployment")
    return deployment


def test_sync_writes_linker_and_startup(fprime_project, cubemx_project):
    exit_code = main(["sync", str(cubemx_project)])

    assert exit_code == 0
    linker_out = fprime_project / "Hardware" / "linker" / "STM32H753xx_FLASH.ld"
    startup_out = fprime_project / "Hardware" / "startup" / "startup_stm32h753xx.s"
    assert linker_out.exists()
    assert startup_out.exists()
    assert "AXI_SRAM" in linker_out.read_text()
    assert "_sdtcm_bss" in startup_out.read_text()


def test_dry_run_writes_nothing(fprime_project, cubemx_project):
    exit_code = main(["sync", str(cubemx_project), "--dry-run"])

    assert exit_code == 0
    assert not (fprime_project / "Hardware" / "linker").exists()
    assert not (fprime_project / "Hardware" / "startup").exists()


def test_fails_cleanly_outside_fprime_project(tmp_path, monkeypatch, cubemx_project, capsys):
    empty_dir = tmp_path / "not_an_fprime_project"
    empty_dir.mkdir()
    monkeypatch.chdir(empty_dir)

    exit_code = main(["sync", str(cubemx_project)])

    assert exit_code == 1
    captured = capsys.readouterr()
    assert "No F' project found" in captured.err
