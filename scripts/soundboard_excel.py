import contextlib
import os
import argparse
import logging
import re
import wave

import coloredlogs
import xlsxwriter

TABLES_PER_ROW = 6
ROW_OFFSET = 1
COL_OFFSET = 1


def get_sequence_for_page(page: int, page_count: int, num_buttons: int) -> list:
    # Compute the maximum sequence length
    max_seq_len = 1
    while num_buttons ** max_seq_len < page_count:
        max_seq_len += 1

    buttons_one_press = 0
    while possible_pages(buttons_one_press + 1, max_seq_len, num_buttons) >= page_count:
        buttons_one_press += 1

    if page < buttons_one_press:
        return [page]
    else:
        seq = []
        remainder = page - buttons_one_press
        for i in range(max_seq_len):
            remainder, mod = divmod(remainder, num_buttons)
            seq.append(mod)
        seq[-1] += buttons_one_press
        return list(reversed(seq))


def possible_pages(buttons_one_press, max_seq_len, num_buttons):
    return buttons_one_press + (num_buttons - buttons_one_press) * (num_buttons ** (max_seq_len - 1))


def get_sound_length(file: str) -> (int, int):
    with contextlib.closing(wave.open(file, 'r')) as f:
        frames = f.getnframes()
        rate = f.getframerate()
        duration = frames / float(rate)
        return round(duration // 60), round(duration % 60)


def directory_excel_generator(root_dir, output_file):
    # Define colors
    colors = ['#FFFF00', '#FF0000', '#000000', '#00FF00', '#000000', '#0000FF']
    symbols = ['⬤', '⬤', '⬤', '⬤', 'ഠ', '⬤']

    colors_alt = ['#FFFF00', '#FF0000', '#000000', '#00FF00', '#FFFFFF', '#0000FF']
    symbols_alt = ['⬤', '⬤', '⬤', '⬤', '⬤', '⬤']

    logging.info(f'Creating workbook {output_file}')
    workbook = xlsxwriter.Workbook(output_file)
    worksheet = workbook.add_worksheet()
    coloronly_formats = [workbook.add_format({'font_color': clr}) for clr in colors]
    color_formats_header = [workbook.add_format({'font_color': clr, 'border': 1, 'bg_color': '#C0C0C0', 'bold': True}) for clr in
                            colors_alt]
    border = workbook.add_format({'border': 1})
    align_right_border = workbook.add_format({'align': 'right', 'border': 1})
    black = workbook.add_format({'font_color': 'black'})
    black_header = color_formats_header[2]
    merged_format = workbook.add_format({'align': 'left', 'bold': True, 'border': 1, 'bg_color': '#C0C0C0'})

    worksheet.set_column(0, COL_OFFSET, 2.14)
    for i in range(ROW_OFFSET):
        worksheet.set_row(i, 15)

    directories = []
    for d in [d for d in os.listdir(root_dir) if os.path.isdir(os.path.join(root_dir, d))]:
        try:
            index, name = re.match(r"(\d+)_(.+)", os.path.basename(d)).groups()
        except AttributeError:
            continue
        directories.append({'index': int(index) - 1, 'name': name, 'path': os.path.join(root_dir, d)})

    for dir_index, subdir in enumerate(sorted(directories, key=lambda x: x['index'])):
        logging.info(f"Processing directory '{subdir}'")

        # Get all files in the directory
        files = [f for f in os.listdir(subdir['path']) if os.path.isfile(os.path.join(subdir['path'], f))]

        # Iterate over the files
        for file in files:
            print(file)

        # Get sorted list of file names (exclude hidden files, keep only the first 6)
        filenames = [""] * 6
        for file in files:
            try:
                m = re.match(r"(\d+)_([^_.]+)", file)
                index = int(m.group(1)) - 1
                name = m.group(2)
                minutes, sec = get_sound_length(os.path.join(subdir['path'], file))
                filenames[index] = f'{name} ({minutes}:{sec:02d})'
            except (IndexError, NameError, AttributeError):
                pass

        # Skip dir with no files
        if all([f == "" for f in filenames]):
            continue

        # Calculate number of the current row and column (5 directories in a row)
        row = (dir_index // TABLES_PER_ROW) * (3 + 2) + ROW_OFFSET  # 2x3 table size with one row padding
        col = (dir_index % TABLES_PER_ROW) * 3 + COL_OFFSET

        worksheet.merge_range(row, col, row, col + 1, 'will be overwritten', merged_format)

        seq = get_sequence_for_page(subdir['index'], len(directories), 6)
        args = []
        for s in seq:
            args.append(color_formats_header[s])
            args.append(symbols_alt[s] + " ")
        worksheet.write_rich_string(row, col, *args, black_header,
                                    str(subdir['index'] + 1) + ". " + subdir['name'], black_header)

        worksheet.set_column(col, col + 1, 19)
        worksheet.set_column(col + 2, col + 2, 2.14)

        for i, filename in enumerate(filenames):
            if i % 2 == 0:
                # color first, then file name
                worksheet.write_rich_string(row + 1 + i // 2, col, coloronly_formats[i], symbols[i] + ' ', black, filename,
                                            border)
            else:
                # file name first, then color
                worksheet.write_rich_string(row + 1 + i // 2, col + 1, black, filename, coloronly_formats[i], ' ' + symbols[i],
                                            align_right_border)


    workbook.close()
    logging.info(f'Successfully created workbook {output_file}')

    os.startfile(output_file)


def main():
    parser = argparse.ArgumentParser(description='Generate Excel from Directory Structure.')
    parser.add_argument('--dir', type=str, default='../wav/soundboard',
                        help='Directory to process. ')
    parser.add_argument('--output', type=str, default='output.xlsx',
                        help='Output Excel file name. default: output.xlsx')

    args = parser.parse_args()

    coloredlogs.install(level='INFO', fmt='%(asctime)s - %(levelname)s - %(message)s')
    directory_excel_generator(args.dir, args.output)


def test_get_sequence_for_page():
    assert get_sequence_for_page(0, 14, 6) == [0]
    assert get_sequence_for_page(1, 14, 6) == [1]
    assert get_sequence_for_page(2, 14, 6) == [2]
    assert get_sequence_for_page(3, 14, 6) == [3]
    assert get_sequence_for_page(4, 14, 6) == [4, 0]
    assert get_sequence_for_page(5, 14, 6) == [4, 1]
    assert get_sequence_for_page(6, 14, 6) == [4, 2]
    assert get_sequence_for_page(7, 14, 6) == [4, 3]
    assert get_sequence_for_page(8, 14, 6) == [4, 4]
    assert get_sequence_for_page(9, 14, 6) == [4, 5]
    assert get_sequence_for_page(10, 14, 6) == [5, 0]
    assert get_sequence_for_page(11, 14, 6) == [5, 1]
    assert get_sequence_for_page(12, 14, 6) == [5, 2]
    assert get_sequence_for_page(13, 14, 6) == [5, 3]


if __name__ == "__main__":
    # test_get_sequence_for_page()
    main()
