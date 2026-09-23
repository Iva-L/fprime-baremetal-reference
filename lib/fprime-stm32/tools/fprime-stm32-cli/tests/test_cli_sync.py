import shutil
from pathlib import Path

import pytest

from fprime_stm32.cli import main

FIXTURE_DIR = Path(__file__).parent / "fixtures" / "cubemx_stm32h753"


@pytest.fixture
def fprime_project(tmp_path, monkeypatch):
    deployment = tmp_path / "MyDeployment"
    (deployment / "Hardware").mkdir(parents=True)
    monkeypatch.chdir(tmp_path / "MyDeployment")
    return deployment


@pytest.fixture
def cubemx_project(fprime_project):
    dest = fprime_project / "Hardware" / "cubemx_stm32h753"
    shutil.copytree(FIXTURE_DIR, dest)
    return dest


def test_sync_writes_linker_startup_and_cmake(fprime_project, cubemx_project):
    exit_code = main(["sync", str(cubemx_project)])

    assert exit_code == 0
    linker_out = fprime_project / "Hardware" / "linker" / "STM32H753xx_FLASH.ld"
    startup_out = fprime_project / "Hardware" / "startup" / "startup_stm32h753xx.s"
    cmake_out = fprime_project / "Hardware" / "CMakeLists.txt"
    assert linker_out.exists()
    assert startup_out.exists()
    assert cmake_out.exists()
    assert "AXI_SRAM" in linker_out.read_text()
    assert "_sdtcm_bss" in startup_out.read_text()

    cmake_text = cmake_out.read_text()
    assert "cubemx_stm32h753/Core/Src/gpio.c" in cmake_text
    assert "STM32H753xx" in cmake_text
    assert 'set(FPRIME_STM32_LINKER_SCRIPT' in cmake_text
    assert "linker/STM32H753xx_FLASH.ld" in cmake_text
    assert "startup/startup_stm32h753xx.s" in cmake_text
    assert "cubemx_stm32h753/Core/Src/stm32h7xx_it.c" in cmake_text  # via FPRIME_STM32_IT_SOURCE


def test_dry_run_writes_nothing(fprime_project, cubemx_project):
    exit_code = main(["sync", str(cubemx_project), "--dry-run"])

    assert exit_code == 0
    assert not (fprime_project / "Hardware" / "linker").exists()
    assert not (fprime_project / "Hardware" / "startup").exists()
    assert not (fprime_project / "Hardware" / "CMakeLists.txt").exists()


def test_fails_cleanly_outside_fprime_project(tmp_path, monkeypatch, cubemx_project, capsys):
    empty_dir = tmp_path / "not_an_fprime_project"
    empty_dir.mkdir()
    monkeypatch.chdir(empty_dir)

    exit_code = main(["sync", str(cubemx_project)])

    assert exit_code == 1
    captured = capsys.readouterr()
    assert "No F' project found" in captured.err


def test_fails_when_cubemx_project_not_under_hardware(fprime_project, tmp_path, capsys):
    outside_project = tmp_path / "cubemx_stm32h753_outside"
    shutil.copytree(FIXTURE_DIR, outside_project)

    exit_code = main(["sync", str(outside_project)])

    assert exit_code == 1
    captured = capsys.readouterr()
    assert "not a direct subdirectory of" in captured.err
