# ESPHome AI Coding Guide (concise)

## Big picture architecture
- ESPHome is a **YAML → Python validation → C++ codegen → firmware** pipeline. Key modules: config/validation in [esphome/config.py](../esphome/config.py) + [esphome/config_validation.py](../esphome/config_validation.py), codegen in [esphome/codegen.py](../esphome/codegen.py) and [esphome/cpp_generator.py](../esphome/cpp_generator.py), runtime core in [esphome/core](../esphome/core).
- Components live under [esphome/components](../esphome/components): `__init__.py` defines config schema + `to_code()`, optional C++ in `[component].h/.cpp`, platform variants in subfolders.
- CLI entrypoint is [esphome/__main__.py](../esphome/__main__.py); dashboard is in [esphome/dashboard](../esphome/dashboard).

## Workflows & commands
- Prefer running tools via `script/run-in-env.py` to use the project venv (see [script](../script)).
- Component build tests: `script/test_build_components` (uses common bus packages in [tests/test_build_components/common](../tests/test_build_components/common)). Avoid `!extend`/`!remove` in component test YAML to keep configs groupable.
- Integration tests: [tests/integration](../tests/integration) use `fixtures/{test_name}.yaml` (auto-mapped from test name). Use `InitialStateHelper` in `state_utils.py` for state subscriptions.
- C++ unit tests: place in [tests/components](../tests/components) and wrap code in `esphome::<component>::testing`; run `script/cpp_unit_test.py`.

## Project-specific conventions
- **Config schema pattern**: use `cv.Schema`, `cv.GenerateID()`, and `cg.new_Pvariable` in `to_code()`; extend with `cv.COMPONENT_SCHEMA` and bus schemas as needed.
- **State during codegen**: store in `CORE.data` (prefer a `@dataclass`); avoid module-level mutable globals.
- **C++ style**: 2-space indent, `this->` member access, `lower_snake_case` methods/fields with trailing `_`, and prefer `protected` fields unless invariants demand `private`.
- **Embedded constraints**: avoid heap allocation after `setup()`; prefer `std::array`, `StaticVector`, `FixedVector` and avoid `std::deque` for RAM/flash reasons.

## Integration points & dependencies
- Adding Python deps: update `requirements*.txt` + [pyproject.toml](../pyproject.toml). C++ libs: update [platformio.ini](../platformio.ini) and use `cg.add_library()`.
- If a component adds build defines via `cg.add_define()`, also add them to [esphome/core/defines.h](../esphome/core/defines.h) for static analysis/IDE support.
