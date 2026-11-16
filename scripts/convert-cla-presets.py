#!/usr/bin/env python3
"""Convert public CLA controls using a matching Parameter_List and preset bank.

This transfers control values, not internal processors or sonic characteristics.
Input/output trims are deliberately excluded (IgnoreOnPresetLoad in the schema).
"""
import argparse
import math
from pathlib import Path
import re
import xml.etree.ElementTree as ET


def convert(schema_path, bank_path, output):
    schema = ET.parse(schema_path).getroot()
    bank = ET.parse(bank_path).getroot()
    if schema.tag != 'Parameter_List' or bank.tag != 'PresetXMLTree':
        raise ValueError('Expected Parameter_List schema and PresetXMLTree bank')
    names = [p.attrib['Name'] for p in schema.findall('Parameter')]
    if len(names) != len(set(names)):
        raise ValueError('Duplicate schema parameter names')
    required = ['PARAM_DI_MODE', 'PARAM_GUITAR_MODE', 'PARAM_BALANCE']
    effects = ['bass', 'treble', 'compress', 'reverb', 'delay', 'pitch']
    required += [f'PARAM_{effect.upper()}{suffix}' for effect in effects for suffix in ['', '_COLOR']]
    if any(name not in names for name in required):
        raise ValueError('Incomplete public control schema')
    results = []
    # Validate the entire bank before writing any output.
    for preset in bank.findall('Preset'):
        if preset.get('GenericType') != 'CLGT':
            raise ValueError('Unsupported plug-in type')
        data = preset.find("PresetData[@Setup='CURRENT']/Parameters[@Type='RealWorld']")
        if data is None:
            raise ValueError('Preset has no CURRENT RealWorld public parameter vector')
        tokens = (data.text or '').split()
        if len(tokens) != len(names):
            raise ValueError('Preset/schema length mismatch; use the matching schema')
        values = dict(zip(names, tokens))
        tree = ET.Element('GuitarRack')
        def put(target, source, low, high, integer=False):
            raw = values[source]
            if raw == '*':
                return None
            value = float(raw)
            if not math.isfinite(value) or not low <= value <= high or (integer and value != int(value)):
                raise ValueError(f'Invalid {source} in {preset.get("Name")}')
            ET.SubElement(tree, 'PARAM', id=target, value=str(value))
            return value
        put('reamp', 'PARAM_DI_MODE', 0, 1, True)
        put('amp', 'PARAM_GUITAR_MODE', 0, 3, True)
        put('balance', 'PARAM_BALANCE', -6, 6)
        for effect in effects:
            put(effect, f'PARAM_{effect.upper()}', -10, 10)
            raw = values[f'PARAM_{effect.upper()}_COLOR']
            if raw != '*':
                color = float(raw)
                if color not in (0, 1, 2, 3):
                    raise ValueError('Color must be Clear/Green/Blue/Red (0..3)')
                ET.SubElement(tree, 'PARAM', id=effect+'On', value='0' if color == 0 else '1')
                if color:
                    # Internal legacy order is Red, Blue, Green; keep host IDs intact.
                    ET.SubElement(tree, 'PARAM', id=effect+'Color', value=str(3-int(color)))
        title = preset.get('Name', 'Unnamed')
        safe = re.sub(r'[^\w .-]', '_', title).strip(' .') or 'Unnamed'
        results.append((safe, tree))
    if not results:
        raise ValueError('No presets found')
    output = Path(output)
    if output.exists() and any(output.iterdir()):
        raise ValueError('Output directory must be empty; refusing to overwrite files')
    output.mkdir(parents=True, exist_ok=True)
    for index, (name, tree) in enumerate(results, 1):
        ET.indent(tree)
        ET.ElementTree(tree).write(output / f'{index:02d} - {name}.xml', encoding='utf-8', xml_declaration=True)
    return len(results)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('schema', type=Path)
    parser.add_argument('bank', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    try:
        count = convert(args.schema, args.bank, args.output)
    except (ValueError, ET.ParseError, OSError) as error:
        parser.exit(1, f'Conversion failed: {error}\n')
    print(f'Converted {count} public-control presets. Internal DSP settings were not imported.')
