import os
import io
import tempfile
import configparser

from .phone import Phone


BTSTACK_CONFIG_FILE = 'bt_stack.conf'
# Needs to always be in UNIX format, hence no os.path.join
BTSTACK_CONFIG_PATH = '/etc/bluetooth/' + BTSTACK_CONFIG_FILE
 
BTSNOOP_FALLBACK_FILE = 'btsnoop_hci.log'
# Needs to always be in UNIX format, hence no os.path.join
BTSNOOP_FALLBACK_PATH = '/sdcard/' + BTSNOOP_FALLBACK_FILE


class SnoopPhone(Phone):

    def __init__(self, serial=None):
        super(SnoopPhone, self).__init__(serial=serial)
        self._tmp_dir = tempfile.mkdtemp()

    def pull_btsnoop(self, dst=None):
        
        btsnoop_path, btsnoop_file = self._locate_btsnoop()

        if not dst:
            dst = os.path.join(self._tmp_dir, btsnoop_file)

        ret = super(SnoopPhone, self).pull(btsnoop_path, dst)
        if ret[0] == 0:
            return dst
        else:
            return None

    def _locate_btsnoop(self):
        tmp_config_path = self._pull_btconfig()
        config = self._parse_btconfig(tmp_config_path)
        try:
            btsnoop_path = config['btsnoopfilename']
            return btsnoop_path, os.path.basename(btsnoop_path)
        except:
            return BTSNOOP_FALLBACK_PATH, BTSNOOP_FALLBACK_FILE

    def _parse_btconfig(self, path):
        if not os.path.exists(path):
            raise ValueError("Failed to read bt_stack.conf")

        class FakeSecHead:
            def __init__(self, fp):
                self.config = io.StringIO()
                self.config.write('[dummy]\n')
                self.config.write(fp.read())
                self.config.seek(0, os.SEEK_SET)

            def __iter__(self):
                return self.config
        
        # Parse key/values
        parser = configparser.SafeConfigParser()
        with open(path, 'r') as f:
            parser.readfp(FakeSecHead(f))
            return dict(parser.items('dummy'))

    def _pull_btconfig(self):
        dst = os.path.join(self._tmp_dir, BTSTACK_CONFIG_FILE)
        retcode, out = super(SnoopPhone, self).pull(BTSTACK_CONFIG_PATH, dst)
        if retcode == 0:
            return dst
        else:
            raise ValueError("Failed to pull bt_stack.conf")