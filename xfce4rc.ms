<?xml version="1.0"?>
<Xfce>
  <Panel orientation="0" layer="0" size="1" popupposition="1" icontheme="Curve" groups="15">
  </Panel>
  <Groups>
    <Group>
      <Popup/>
      <Control id="-2" popup="0">
        <Command term="0" sn="0">xfterm4</Command>
        <Tooltip>Terminal</Tooltip>
        <Icon id="12"/>
      </Control>
    </Group>
    <Group>
      <Popup>
        <Item>
          <Caption>Konqueror</Caption>
          <Command term="0" sn="0">konqueror</Command>
          <Tooltip>Pengurus Fail Konqueror</Tooltip>
          <Icon id="2"/>
        </Item>
        <Item>
          <Caption>Nautilus</Caption>
          <Command term="0" sn="0">nautilus --no-desktop</Command>
          <Tooltip>Pengurus Fail Nautilus</Tooltip>
          <Icon id="2"/>
        </Item>
      </Popup>
      <Control id="-2" popup="1">
        <Command term="0" sn="1">xffm</Command>
        <Tooltip>Pengurus fail</Tooltip>
        <Icon id="2"/>
      </Control>
    </Group>
    <Group>
      <Popup/>
      <Control id="-2" popup="0">
        <Command term="0" sn="0">mozilla -splash</Command>
        <Tooltip>Pelayar Web Mozilla</Tooltip>
        <Icon id="7"/>
      </Control>
    </Group>
    <Group>
      <Popup/>
      <Control id="-1" filename="libmailcheck.so">
        <Mailcheck interval="30">
          <Command term="0" sn="0">mozilla -mail -splash</Command>
        </Mailcheck>
      </Control>
    </Group>
    <Group>
      <Popup/>
      <Control id="-2" popup="0">
        <Command term="0" sn="0">xmms</Command>
        <Tooltip>Pemain muzik</Tooltip>
        <Icon id="6"/>
      </Control>
    </Group>
    <Group>
      <Popup/>
      <Control id="-1" filename="libseparator.so"/>
    </Group>
    <Group>
      <Popup/>
      <Control id="-1" filename="libpager.so"/>
    </Group>
    <Group>
      <Popup/>
      <Control id="-1" filename="libseparator.so"/>
    </Group>
    <Group>
      <Popup>
        <Item>
          <Caption>Screensaver</Caption>
          <Command term="0" sn="0">xscreensaver-demo</Command>
          <Tooltip>Screensaver</Tooltip>
          <Icon id="3"/>
        </Item>
      </Popup>
      <Control id="-2" popup="1">
        <Command term="0" sn="0">xfce-setting-show</Command>
        <Tooltip>Tetapan</Tooltip>
        <Icon id="3"/>
      </Control>
    </Group>
    <Group>
      <Popup>
        <Item>
          <Caption>Pengurusan Percetakan</Caption>
          <Command term="0" sn="0">xfprint-manager</Command>
          <Tooltip>Pengurusan Percetakan</Tooltip>
          <Icon id="9"/>
        </Item>
      </Popup>
      <Control id="-2" popup="1">
        <Command term="0" sn="1">xfprint4</Command>
        <Tooltip>Seret fail untuk cetak</Tooltip>
        <Icon id="9"/>
      </Control>
    </Group>
    <Group>
      <Popup/>
      <Control id="-2" popup="0">
        <Command term="0" sn="0">xfhelp4</Command>
        <Tooltip>Manual</Tooltip>
        <Icon id="5"/>
      </Control>
    </Group>
    <Group>
      <Popup/>
      <Control id="-1" filename="libseparator.so"/>
    </Group>
    <Group>
      <Popup/>
      <Control id="-1" filename="libsystembuttons.so" button1="0" button2="1" showtwo="1"/>
    </Group>
    <Group>
      <Popup/>
      <Control id="-1" filename="libseparator.so"/>
    </Group>
    <Group>
      <Popup/>
      <Control id="-1" filename="libclock.so">
        <XfceClock Clock_type="2" Toggle_military="1" Toggle_am_pm="1" Toggle_secs="0"/>
      </Control>
    </Group>
  </Groups>
</Xfce>
