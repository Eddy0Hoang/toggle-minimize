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
