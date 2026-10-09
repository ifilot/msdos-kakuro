#!/usr/bin/env python3
"""Regressions for editor-wrapped help paragraphs and heading boundaries."""
from pathlib import Path
import sys
import textwrap
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from generate_documents import wrap_document


class DocumentTests(unittest.TestCase):
    def test_headings_do_not_capture_following_body(self):
        self.assertEqual(wrap_document('# Rules\nFill the grid\nwith digits.\n'),
                         [('Rules', 1), ('Fill the grid with digits.', 0)])

    def test_source_wrapping_does_not_change_display(self):
        paragraph = ('Each cream cell contains a digit from 1 to 9. '
                     'Clues give the sum of the cells in a run. '
                     'Crossing runs help you decide the order.')
        self.assertEqual(wrap_document(paragraph),
                         wrap_document(textwrap.fill(paragraph, width=80)))

    def test_bullets_stay_separate_and_reflow_continuations(self):
        self.assertEqual(wrap_document('- Match every\n  clue sum.\n- Never repeat a digit.\n'),
                         [('- Match every clue sum.', 0), ('- Never repeat a digit.', 0)])
        wrapped = wrap_document('- Digits in a run must be unique.', width=20)
        self.assertEqual(wrapped, [('- Digits in a run', 0), ('  must be unique.', 0)])

    def test_help_source_is_80_columns_with_separate_titles(self):
        source = (ROOT / 'assets/documents/HELP.TXT').read_text()
        self.assertLessEqual(max(map(len, source.splitlines())), 80)
        lines = wrap_document(source)
        self.assertEqual(lines[0], ('Welcome to Kakuro', 1))
        self.assertEqual([text for text, heading in lines if heading],
                         ['Welcome to Kakuro', 'Reading the clues', 'The two rules',
                          'Writing and erasing', 'Finishing a puzzle',
                          'Returning to the journal', 'Choosing a puzzle',
                          'A useful first step', 'Mouse controls', 'Music and sound'])
        self.assertTrue(next(text for text, heading in lines if text and not heading)
                        .startswith('Kakuro combines'))


if __name__ == '__main__':
    unittest.main()
