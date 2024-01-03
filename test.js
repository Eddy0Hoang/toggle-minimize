const { BrowserWindow, app, Menu } = require("electron");
const binding = require('./bin/toggle-minimize.node')

let parentHandle = 0;
app.whenReady().then(() => {
  console.log("ready");
  const win = new BrowserWindow({
    width: 400,
    height: 400,
    show: true,
  });
  win.loadURL("https://baidu.com");
  win.setMenu(
    Menu.buildFromTemplate([
      {
        label: "disable",
        click: () => {
          const handle = win.getNativeWindowHandle();
          const res = binding.disableMinimize(handle);
          console.log("res", res);
          if (res.code) {
            parentHandle = res.handle;
          }
        },
      },
      {
        label: "enable",
        click: () => {
          const handle = win.getNativeWindowHandle();
          const res = binding.enableMinimize(handle, parentHandle);
          console.log("enable res", res);
        },
      },
    ])
  );
});
