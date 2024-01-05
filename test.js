const { BrowserWindow, app, Menu } = require("electron");
const binding = require("./bin/win32-x64-116/toggle-minimize.node");

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
      {
        label: "hook",
        click: () => {
          const res = binding.hookMouseMove((val) => {
            // console.log("val:", val);
          });
          const res2 = binding.hookKeyboard((val) => {
            console.log("kb val", val);
          });
          console.log("hook res", res, res2);
        },
      },
      {
        label: "unhook",
        click: () => {
          const res = binding.unhookMouse();
          const res2 = binding.unhookKeyboard();
          console.log("unhook res", res, res2);
        },
      },
    ])
  );
});

process.on("uncaughtException", (e) => {
  console.log("e", e);
});

app.on("will-quit", () => {
  binding.unhookMouse();
  binding.unhookKeyboard();
});
