import copy
import unittest
from unittest.mock import patch
from model import DEFAULT,validate,commands,header
from device import Device

class Tests(unittest.TestCase):
    def test_valid_design_and_header(self):
        self.assertEqual(validate(DEFAULT),DEFAULT)
        self.assertEqual(commands(DEFAULT)[-1],'design commit')
        self.assertIn('.w=91,.h=15',header(DEFAULT))
        self.assertTrue(all(len(x)<127 for x in commands(DEFAULT)))
    def test_reject_invalid(self):
        cases=[]
        for field,value in [('x',-1),('x',120),('w',113),('y',40)]:
            scene=copy.deepcopy(DEFAULT); scene['elements'][0][field]=value; cases.append(scene)
        for text in ('','a\nb','é','a'*49):
            scene=copy.deepcopy(DEFAULT); scene['elements'][1]['text']=text; cases.append(scene)
        scene=copy.deepcopy(DEFAULT); scene['elements']*=7; cases.append(scene)
        for scene in cases:
            with self.subTest(scene=scene),self.assertRaises(ValueError): validate(scene)
    def test_transaction_cancels(self):
        device=Device(); calls=[]
        def command(line):
            calls.append(line)
            if line.startswith('design crest'): raise RuntimeError('Rejected')
        device.command=command
        with self.assertRaises(RuntimeError): device.upload(DEFAULT)
        self.assertNotIn('design commit',calls); self.assertEqual(calls[-1],'design cancel')
    def test_escaped_c_text(self):
        scene=copy.deepcopy(DEFAULT); scene['elements'][1]['text']='Quote " and \\'
        self.assertIn('\\"',header(scene)); self.assertIn('\\\\',header(scene))

if __name__=='__main__': unittest.main()
