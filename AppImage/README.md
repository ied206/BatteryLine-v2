# Linux AppImage packaging

## Instructions

Download these tools and place them into single directory.

- [linuxdeploy](https://github.com/linuxdeploy/linuxdeploy)
- [linuxdeploy-qt](https://github.com/linuxdeploy/linuxdeploy-plugin-qt)
- [AppImageKit](https://github.com/AppImage/AppImageKit)

Add `executable` permission after downloading them.

```sh
# If 'qmake' is located in '$HOME/Qt/6.6.0/gcc_64/bin/qmake'
export QMAKE=$HOME/Qt/6.6.0/gcc_64/bin/qmake
AppImage/linuxdeploy-x86_64.AppImage --appdir BatteryLine.AppDir --plugin qt -e BatteryLine -d BatteryLine.desktop -i BatteryLine.png
AppImage/appimagetool-x86_64.AppImage BatteryLine.AppDir
```
