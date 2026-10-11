import asyncio
from bleak import BleakClient

import pyautogui
import win32gui
import win32process
import psutil


# ESP32 BLE地址
BLE_ADDRESS = "14:C1:9F:32:1B:DA"

# Notify特征UUID
TX_UUID = "0000ff02-0000-1000-8000-00805f9b34fb"


# 获取当前前台窗口对应的进程名
def get_foreground_process():

    hwnd = win32gui.GetForegroundWindow()

    _, pid = win32process.GetWindowThreadProcessId(hwnd)

    try:
        process = psutil.Process(pid)
        return process.name().upper()
    except Exception:
        return ""


def ppt_jump(page):

    print(f"PPT跳转到第 {page} 页")
    pyautogui.write(str(page))
    pyautogui.press("enter")


def word_jump(page):

    print(f"Word跳转到第 {page} 页")

    pyautogui.hotkey("ctrl", "g")

    pyautogui.write(str(page))

    pyautogui.press("enter")

    pyautogui.press("esc")


def pdf_jump(page):

    print(f"PDF跳转到第 {page} 页")

    pyautogui.hotkey("ctrl", "shift", "n")

    pyautogui.write(str(page))

    pyautogui.press("enter")


PROCESS_MAP = {
    "POWERPNT.EXE": ppt_jump,
    "WINWORD.EXE": word_jump,
    "ACROBAT.EXE": pdf_jump,
    "ACRORD32.EXE": pdf_jump,
}


def process_page(page):

    process_name = get_foreground_process()

    print("当前进程:", process_name)

    handler = PROCESS_MAP.get(process_name)

    if handler:
        handler(page)
    else:
        print("当前软件暂不支持")


def notification_handler(sender, data):

    try:

        msg = data.decode("utf-8").strip()

        print("收到BLE:", msg)

        # PAGE:5
        if msg.startswith("PAGE:"):

            page = int(msg.split(":")[1])

            process_page(page)

    except Exception as e:

        print("处理失败:", e)


async def main():

    print("正在连接ESP32...")

    async with BleakClient(BLE_ADDRESS) as client:

        if not client.is_connected:
            print("连接失败")
            return

        print("连接成功")

        await client.start_notify(
            TX_UUID,
            notification_handler
        )

        print("开始监听BLE通知...")
        print("等待 PAGE:x 命令")

        while True:
            await asyncio.sleep(1)


if __name__ == "__main__":

    asyncio.run(main())


