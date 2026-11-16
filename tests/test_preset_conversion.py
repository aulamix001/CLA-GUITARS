import importlib.util
from pathlib import Path
import tempfile
import unittest
import xml.etree.ElementTree as ET

spec = importlib.util.spec_from_file_location('converter', Path(__file__).parents[1] / 'scripts/convert-cla-presets.py')
converter = importlib.util.module_from_spec(spec)
spec.loader.exec_module(converter)


class PresetConversion(unittest.TestCase):
    def make_input(self, directory, color='1', number='1', count_delta=0):
        names = ['PARAM_INPUT_GAIN', 'PARAM_OUTPUT_GAIN', 'PARAM_DI_MODE', 'PARAM_GUITAR_MODE', 'PARAM_BALANCE']
        values = ['8', '-8', '1', '2', '-3.5']
        for effect in ['BASS', 'TREBLE', 'COMPRESS', 'REVERB', 'DELAY', 'PITCH']:
            names += ['PARAM_'+effect, 'PARAM_'+effect+'_COLOR']
            values += [number, color]
        schema = ET.Element('Parameter_List')
        for name in names:
            ET.SubElement(schema, 'Parameter', Name=name)
        bank = ET.Element('PresetXMLTree')
        preset = ET.SubElement(bank, 'Preset', GenericType='CLGT', Name='Test / preset')
        params = ET.SubElement(ET.SubElement(preset, 'PresetData', Setup='CURRENT'), 'Parameters', Type='RealWorld')
        params.text = ' '.join(values[:len(values)+count_delta])
        for name, tree in [('schema.xml', schema), ('bank.xml', bank)]:
            ET.ElementTree(tree).write(directory/name)
        return directory/'schema.xml', directory/'bank.xml', directory/'output'

    def test_controls_colors_and_trim_preservation(self):
        for color, expected in [('0', None), ('1', '2'), ('2', '1'), ('3', '0')]:
            with tempfile.TemporaryDirectory() as temp:
                paths = self.make_input(Path(temp), color)
                self.assertEqual(converter.convert(*paths), 1)
                tree = ET.parse(next(paths[2].glob('*.xml'))).getroot()
                values = {p.get('id'): p.get('value') for p in tree}
                self.assertEqual(values['amp'], '2.0')
                self.assertEqual(values['reamp'], '1.0')
                self.assertEqual(values['balance'], '-3.5')
                self.assertEqual(values.get('bassColor'), expected)
                self.assertEqual(values['bassOn'], '0' if color=='0' else '1')
                self.assertNotIn('input', values)
                self.assertNotIn('output', values)
                with self.assertRaises(ValueError):
                    converter.convert(*paths)  # Never overwrite user files.

    def test_rejects_invalid_and_incomplete_bank_without_output(self):
        for color, number, delta in [('4', '1', 0), ('1', 'nan', 0), ('1', '1', -1)]:
            with tempfile.TemporaryDirectory() as temp:
                paths = self.make_input(Path(temp), color, number, delta)
                with self.assertRaises(ValueError):
                    converter.convert(*paths)
                self.assertFalse(paths[2].exists())

    def test_asterisk_leaves_control_unchanged(self):
        with tempfile.TemporaryDirectory() as temp:
            paths = self.make_input(Path(temp), '*', '*')
            converter.convert(*paths)
            values = {p.get('id') for p in ET.parse(next(paths[2].glob('*.xml'))).getroot()}
            self.assertNotIn('bass', values)
            self.assertNotIn('bassOn', values)


if __name__ == '__main__':
    unittest.main()
