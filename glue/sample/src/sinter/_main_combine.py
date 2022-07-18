import sys
from typing import List

from sinter._csv_out import CSV_HEADER
from sinter._existing_data import ExistingData


def main_combine(*, command_line_args: List[str]):
    if command_line_args:
        total = ExistingData()
        for path in command_line_args:
            total += ExistingData.from_file(path)
    else:
        total = ExistingData.from_file(sys.stdin)

    print(CSV_HEADER)
    for value in total.data.values():
        print(value.to_csv_line())
