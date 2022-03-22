import sys
from typing import Optional, List


def main(*, command_line_args: Optional[List[str]] = None):
    if command_line_args is None:
        command_line_args = sys.argv[1:]

    mode = command_line_args[0] if command_line_args else None
    if mode == 'combine':
        from simmer.main_combine import main_combine
        return main_combine(command_line_args=command_line_args[1:])
    if mode == 'collect':
        from simmer.main_collect import main_collect
        return main_collect(command_line_args=command_line_args[1:])
    if mode == 'plot':
        from simmer.main_plot import main_plot
        return main_plot(command_line_args=command_line_args[1:])
    if command_line_args and not command_line_args[0].startswith('-'):
        print(f"\033[31mUnrecognized command: simmer {command_line_args[0]}\033[0m\n", file=sys.stderr)
    else:
        print(f"\033[31mDidn't specify a command.\033[0m\n", file=sys.stderr)
    print(f"Known commands are:\n"
          f"    simmer collect\n"
          f"    simmer combine\n"
          f"    simmer plot\n"
          f"", file=sys.stderr)
    sys.exit(1)


if __name__ == '__main__':
    main()
