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


def directory_excel_generator(root_dir: str, output_file: str, flat: bool = False, add_duration: bool = False):
    # Define colors
    colors = ['#FFFF00', '#FF0000', '#000000', '#00FF00', '#000000', '#0000FF']
    symbols = ['⬤', '⬤', '⬤', '⬤', 'ഠ', '⬤']

    colors_alt = ['#FFFF00', '#FF0000', '#000000', '#00FF00', '#FFFFFF', '#0000FF']
    symbols_alt = ['⬤', '⬤', '⬤', '⬤', '⬤', '⬤']

    caption_format_base = {'border': 1, 'bg_color': '#C0C0C0', 'bold': True}

    logging.info(f'Creating workbook {output_file}')
    workbook = xlsxwriter.Workbook(output_file)
    worksheet = workbook.add_worksheet()
    coloronly_formats = [workbook.add_format({'font_color': clr}) for clr in colors]
    color_formats_header = [workbook.add_format({'font_color': clr, 'align': 'center'} | caption_format_base) for clr in
                            colors_alt]
    border = workbook.add_format({'border': 1})
    border_no_right = workbook.add_format({'border': 1})
    border_no_right.set_right(0)
    border_no_left = workbook.add_format({'border': 1})
    border_no_left.set_left(0)
    align_right_border = workbook.add_format({'align': 'right', 'border': 1})
    black = workbook.add_format({'font_color': 'black'})
    black_header = color_formats_header[2]
    merged_format = workbook.add_format(caption_format_base | {'align': 'left'})

    caption_left = workbook.add_format(caption_format_base)
    caption_left.set_right(0)
    caption_right = workbook.add_format(caption_format_base | {'align': 'center'})
    caption_right.set_right(0)

    text_colored_border = [workbook.add_format({'border': 1}) for _ in colors]
    text_colored_border_alt = [workbook.add_format({'border': 1, 'bg_color': '#E0E0E0'}) for _ in colors]
    for i, f in enumerate(text_colored_border):
        f.set_left_color(colors[i])
        f.set_left(5)
    for i, f in enumerate(text_colored_border_alt):
        f.set_left_color(colors[i])
        f.set_left(5)

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

    if flat:
        worksheet.set_column(COL_OFFSET, COL_OFFSET, 12)
        worksheet.set_column(COL_OFFSET + 1, COL_OFFSET + 1, 4.5)
        worksheet.set_column(COL_OFFSET + 2, COL_OFFSET + 8, 12)
        for i in range(6):
            worksheet.write(ROW_OFFSET, COL_OFFSET + i + 2, symbols_alt[i], color_formats_header[i])

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

                filenames[index] = name
                if add_duration:
                    minutes, sec = get_sound_length(os.path.join(subdir['path'], file))
                    filenames[index] += f' ({minutes}:{sec:02d})'
            except (IndexError, NameError, AttributeError):
                pass

        # Skip dir with no files
        if all([f == "" for f in filenames]):
            continue

        # Header for page with colored buttons
        page_seq = get_sequence_for_page(subdir['index'], len(directories), 6)

        if flat:
            row = dir_index + ROW_OFFSET + 1
            col = COL_OFFSET

            worksheet.write(row, col, str(subdir['index'] + 1) + ". " + subdir['name'], caption_left)
            sequence_richtext = []
            for s in page_seq:
                sequence_richtext.append(color_formats_header[s])
                sequence_richtext.append(symbols_alt[s] + " ")
            worksheet.write_rich_string(row, col + 1, *sequence_richtext, '\u200b', caption_right)
        else:
            # Calculate number of the current row and column (5 directories in a row)
            row = (dir_index // TABLES_PER_ROW) * (3 + 2) + ROW_OFFSET  # 2x3 table size with one row padding
            col = (dir_index % TABLES_PER_ROW) * 3 + COL_OFFSET
            worksheet.merge_range(row, col, row, col + 1, 'will be overwritten', merged_format)
            sequence_richtext = []
            for s in page_seq:
                sequence_richtext.append(color_formats_header[s])
                sequence_richtext.append(symbols_alt[s] + " ")
            worksheet.write_rich_string(row, col, *sequence_richtext, black_header,
                                        str(subdir['index'] + 1) + ". " + subdir['name'], black_header)

        if not flat:
            worksheet.set_column(col, col + 1, 19)
            worksheet.set_column(col + 2, col + 2, 2.14)

        for i, filename in enumerate(filenames):
            if filename != "":
                if flat:
                    # only file name
                    worksheet.write(row, col + i + 2, filename, text_colored_border[i] if row % 2 == 0 else text_colored_border_alt[i])
                else:
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
    parser.add_argument('--dir', type=str, default='../wav/soundboard', help='Directory to process. ')
    parser.add_argument('--output', type=str, default='output.xlsx', help='Output Excel file name. default: output.xlsx')
    parser.add_argument("--flat", action='store_true', help="Make list flat instead of 2x3 tables")
    parser.add_argument("--add_duration", action='store_true', help="Add sound duration to names")

    args = parser.parse_args()

    coloredlogs.install(level='INFO', fmt='%(asctime)s - %(levelname)s - %(message)s')
    directory_excel_generator(args.dir, args.output, args.flat, args.add_duration)


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
