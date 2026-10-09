#!/usr/bin/env python3
"""Check the distributed Quickmenu file with an independent bitmap decoder."""
from pathlib import Path
import struct
import unittest

ICON = Path(__file__).resolve().parents[1] / 'assets/quickmenu/KAKURO.ICC'


def pixel(bitmap, x, y):
    row = bitmap[4+y*16:4+(y+1)*16]
    return sum(((row[plane*4+x//8] >> (7-x%8)) & 1) << plane
               for plane in range(4))


class IconTests(unittest.TestCase):
    def test_import_layout_and_colors(self):
        data = ICON.read_bytes()
        self.assertEqual(len(data), 1032)
        image, mask = data[:516], data[516:]
        self.assertEqual(struct.unpack('<HH', image[:4]), (31, 31))
        self.assertEqual(mask[:4], image[:4])
        # Exact EGA indices at recognizable parts of the puzzle tile.
        self.assertEqual(pixel(image, 2, 2), 14)  # yellow frame highlight
        self.assertEqual(pixel(image, 3, 3), 6)   # brown frame
        self.assertEqual(pixel(image, 12, 12), 15)  # white answer cell
        self.assertEqual(pixel(image, 5, 5), 8)   # dark gray blocked cell
        for y in range(32):
            for x in range(32):
                self.assertIn(pixel(mask, x, y), (0, 15))
        self.assertEqual(pixel(mask, 0, 0), 0)
        self.assertEqual(pixel(mask, 31, 31), 0)
        self.assertEqual(pixel(mask, 1, 1), 15)
        self.assertEqual(pixel(mask, 3, 29), 15)  # one-pixel black shadow
        self.assertEqual(pixel(mask, 4, 30), 0)


if __name__ == '__main__':
    unittest.main()
