from typing import List

from simmer.csv_out import CSV_HEADER
from simmer.existing_data import ExistingData


def main_combine(*, command_line_args: List[str]):
    total = ExistingData()
    for path in command_line_args:
        total += ExistingData.from_file(path)

    print(CSV_HEADER)
    for value in total.data.values():
        print(value.to_csv_line())
