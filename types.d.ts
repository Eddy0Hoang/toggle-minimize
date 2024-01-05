export type ExecResult = {
  done: boolean;
  msg?: string;
  handle?: number;
};
export function disableMinimize(targetWindowHandle: Buffer): ExecResult;
export function enableMinimize(
  targetWindowHandle: Buffer,
  parentWindowHandle: number
): ExecResult;
export function hookMouseMove(
  cb: (val: { x: number; y: number }) => void
): ExecResult;
export function unhookMouse(): ExecResult;

export function hookKeyboard(
  cb: (val: { evName: "keyup" | "keydown"; code: number }) => void
): ExecResult;
export function unhookKeyboard(): ExecResult;
