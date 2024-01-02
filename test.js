const { BrowserWindow, app, Menu } = require("electron");
const binding = require("bindings")("electron-disable-minimize");

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
          const res = binding.DisableMinimize(handle);
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
          const res = binding.EnableMinimize(handle, parentHandle + '');
          console.log("enable res", res);
        },
      },
    ])
  );
});
