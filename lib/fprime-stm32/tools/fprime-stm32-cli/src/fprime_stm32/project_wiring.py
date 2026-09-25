import re

from fprime_stm32.errors import CliError

_PROJECT_RE = re.compile(r"^project\([^\n]*\)[ \t]*$", re.MULTILINE)
_ENABLE_ASM_RE = re.compile(r"enable_language\(\s*ASM\s*\)")
_CONFIG_SUBDIR_RE = re.compile(r"add_fprime_subdirectory\([^)]*config[^)]*\)", re.IGNORECASE)
_HARDWARE_SUBDIR_RE = re.compile(r"add_fprime_subdirectory\([^)]*Hardware[^)]*\)")
_RESTRICT_PLATFORMS_RE = re.compile(r"restrict_platforms\(")
_REGISTER_DEPLOYMENT_RE = re.compile(r"register_fprime_deployment\(([^)]*)\)", re.DOTALL)
_SOURCES_LIST_RE = re.compile(r"(SOURCES\b)((?:.|\n)*?)(?=\n[ \t]*DEPENDS\b|\Z)")
_DEPENDS_LIST_RE = re.compile(r"(DEPENDS\b)((?:.|\n)*?)(?=\n[ \t]*\)|\Z)")
_LINK_OPTIONS_RE = re.compile(r"target_link_options\([^)]*FPRIME_STM32_LINKER_SCRIPT[^)]*\)", re.DOTALL)
_REGISTER_MODULE_RE = re.compile(r"register_fprime_module\(([^)]*)\)", re.DOTALL)

_TARGET_LINK_OPTIONS_BLOCK = """
target_link_options(${FPRIME_CURRENT_MODULE} PRIVATE
    "-T${FPRIME_STM32_LINKER_SCRIPT}"
    "-Wl,--gc-sections"
    "-Wl,--undefined=_Znwj"
    "-Wl,--undefined=_Znaj"
    "-Wl,--undefined=_Znwjl"
    "-Wl,--undefined=_Znajl"
    "-Wl,--undefined=_ZnwjRKSt9nothrow_t"
    "-Wl,--undefined=_ZnajRKSt9nothrow_t"
    "-Wl,--undefined=_ZdlPv"
    "-Wl,--undefined=_ZdaPv"
    "-Wl,--undefined=_ZdlPvl"
    "-Wl,--undefined=_ZdaPvl"
    "-Wl,--undefined=_ZdlPvj"
    "-Wl,--undefined=_ZdaPvj"
    "--specs=nosys.specs"
)
"""


def patch_root_cmakelists(text: str) -> tuple[str, list[str]]:
    actions = []
    patched = text

    if _ENABLE_ASM_RE.search(patched) is None:
        match = _PROJECT_RE.search(patched)
        if match is None:
            raise CliError("Could not find a project(...) call in the root CMakeLists.txt")
        insert_at = match.end()
        patched = patched[:insert_at] + "\nenable_language(ASM)" + patched[insert_at:]
        actions.append("Enabled the ASM language (required to compile Hardware/startup/*.s)")

    if _CONFIG_SUBDIR_RE.search(patched) is None:
        patched = patched.rstrip("\n") + '\nadd_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/config")\n'
        actions.append("Added config/ to the project's build graph")

    return patched, actions


def patch_namespace_cmakelists(text: str) -> tuple[str, list[str]]:
    actions = []
    patched = text

    if _HARDWARE_SUBDIR_RE.search(patched) is None:
        patched = patched.rstrip("\n") + '\nadd_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Hardware")\n'
        actions.append("Added Hardware/ to the project's build graph")

    return patched, actions


def ensure_deployment_registered(
    root_text: str, namespace_text: str, namespace_name: str, deployment_name: str
) -> tuple[str, str, list[str], list[str]]:
    """Make sure Deployments/<deployment_name> is add_fprime_subdirectory'd somewhere reachable,
    matching fprime's convention of registering deployments in the ROOT CMakeLists.txt (the
    namespace CMakeLists.txt registers Components/config/Hardware instead).

    `fprime-util new --deployment`'s own interactive prompt sometimes appends a registration to
    the NAMESPACE CMakeLists.txt with a path that's only valid from ROOT context (it repeats the
    namespace directory name as a prefix, e.g. "${CMAKE_CURRENT_LIST_DIR}/Stm32h7Project/Deployments/..."
    inside Stm32h7Project's own CMakeLists.txt, where CMAKE_CURRENT_LIST_DIR is already
    Stm32h7Project/) -- CMake then can't find that nonexistent doubly-nested directory. This
    detects and removes exactly that broken line, then ensures a correct one exists in root.
    """
    root_actions: list[str] = []
    namespace_actions: list[str] = []

    escaped_namespace = re.escape(namespace_name)
    escaped_deployment = re.escape(deployment_name)

    broken_re = re.compile(
        r'[ \t]*add_fprime_subdirectory\(\s*"\$\{CMAKE_CURRENT_LIST_DIR\}/'
        rf'{escaped_namespace}/Deployments/{escaped_deployment}/?"\s*\)\s*\n?'
    )
    broken_match = broken_re.search(namespace_text)
    if broken_match is not None:
        namespace_text = namespace_text[: broken_match.start()] + namespace_text[broken_match.end() :]
        namespace_actions.append(
            f"Removed a broken Deployments/{deployment_name} registration ('new --deployment' "
            "sometimes adds this to the namespace CMakeLists.txt with a path that only resolves "
            "correctly from the root CMakeLists.txt)"
        )

    already_registered_re = re.compile(rf"add_fprime_subdirectory\([^)]*Deployments/{escaped_deployment}\b[^)]*\)")
    if already_registered_re.search(root_text) is None and already_registered_re.search(namespace_text) is None:
        registration_path = f"${{CMAKE_CURRENT_LIST_DIR}}/{namespace_name}/Deployments/{deployment_name}/"
        root_text = root_text.rstrip("\n") + f'\nadd_fprime_subdirectory("{registration_path}")\n'
        root_actions.append(f"Registered Deployments/{deployment_name} in the project's build graph")

    return root_text, namespace_text, root_actions, namespace_actions


def _patch_call_list(text: str, list_re: re.Pattern, insert_line: str) -> tuple[str, bool]:
    match = list_re.search(text)
    if match is None:
        return text, False
    return text[: match.end()] + insert_line + text[match.end() :], True


def patch_deployment_cmakelists(text: str) -> tuple[str, list[str]]:
    actions = []
    patched = text

    if _RESTRICT_PLATFORMS_RE.search(patched) is None:
        insert_at = 0
        for line in patched.splitlines(keepends=True):
            if line.strip() == "" or line.lstrip().startswith("#"):
                insert_at += len(line)
            else:
                break
        patched = patched[:insert_at] + "restrict_platforms(stm32h7)\n\n" + patched[insert_at:]
        actions.append("Restricted this deployment to the stm32h7 platform")

    deployment_match = _REGISTER_DEPLOYMENT_RE.search(patched)
    if deployment_match is None:
        raise CliError(
            "Could not find a register_fprime_deployment(...) call in this deployment's "
            "CMakeLists.txt - unrecognized deployment template"
        )

    for var in ("${FPRIME_STM32_STARTUP_SOURCE}", "${FPRIME_STM32_IT_SOURCE}"):
        if var in patched:
            continue
        patched, ok = _patch_call_list(patched, _SOURCES_LIST_RE, f'\n        "{var}"')
        if ok:
            actions.append(f"Added {var} to the deployment's SOURCES")

    for dep in ("FprimeStm32", "FprimeStm32Config", "FprimeStm32Allocator", "Os_Baremetal_OverrideNewDelete"):
        if re.search(rf"\b{dep}\b", patched):
            continue
        patched, ok = _patch_call_list(patched, _DEPENDS_LIST_RE, f"\n        {dep}")
        if ok:
            actions.append(f"Added {dep} to the deployment's DEPENDS")

    if _LINK_OPTIONS_RE.search(patched) is None:
        # Re-search on the current text: SOURCES/DEPENDS insertions above may have
        # shifted where the call's closing ")" now sits.
        deployment_match = _REGISTER_DEPLOYMENT_RE.search(patched)
        insert_at = deployment_match.end()
        patched = patched[:insert_at] + "\n" + _TARGET_LINK_OPTIONS_BLOCK + patched[insert_at:]
        actions.append(
            "Appended target_link_options(...) with the linker script and the operator "
            "new/delete undefined-symbol list"
        )

    return patched, actions


def patch_top_cmakelists(text: str) -> tuple[str, list[str]]:
    actions = []
    patched = text

    if not re.search(r"\bFprimeStm32Allocator\b", patched):
        module_match = _REGISTER_MODULE_RE.search(patched)
        if module_match is None:
            raise CliError(
                "Could not find a register_fprime_module(...) call in this deployment's "
                "Top/CMakeLists.txt - unrecognized template"
            )
        patched, ok = _patch_call_list(patched, _DEPENDS_LIST_RE, "\n        FprimeStm32Allocator")
        if ok:
            actions.append("Added FprimeStm32Allocator to Top/CMakeLists.txt's DEPENDS")

    return patched, actions
