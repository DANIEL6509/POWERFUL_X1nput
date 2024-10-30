# Software Description

<img src="https://github.com/DANIEL6509/POWERFUL_X1nput/blob/main/STEP_0.jpg?raw=true" alt="My Image1" style="width: 50%; height: auto;" />


This project consists of two main components: 
1. Reading the game output and processing it to obtain game statistics.
2. Using X1nput to apply the defined statistics to the controller triggers. Here’s a detailed explanation:

1. Reading the game output and calculating game statistics, with the following features:
   - a. Controlling the right vibration trigger based on RPM (as normalized RPM increases, the vibration intensity increases, creating a strong feedback sensation, achieved through an exponential function).
   - b. Controlling the left vibration trigger when ABS is activated (monitors wheel slip using the square root of the sum of the squares of the four wheels’ slip).
   - c. Generating vibrations upon collisions (monitors changes in acceleration; if the acceleration values in any direction are too high, a "BUMP" vibration is triggered).
   - d. Providing a vibration reminder when in reverse gear (monitors gear position).
   - e. Ensuring that the triggers do not vibrate when the game is paused (if the engine RPM is 0, vibrations are paused. Thanks to [richstokes](https://github.com/richstokes/Forza-data-tools) for the inspiration).

2. Utilizing X1nput to apply the defined statistics to the controller triggers, modified from [araghon007](https://github.com/araghon007/X1nput). Here’s the original statement:  
   *X1nput is a reimplementation of Xinput using the Windows.Gaming.Input API, which provides better support for Xbox One controllers, including impulse triggers.*

## Installation

1. **Simple Usage**:
   - a. If you wish to use the preset parameters, copy `XInput1_3.dll` and `X1nput.ini` into the game folder (usually located at `C:\Program Files (x86)\Steam\steamapps\common\ForzaHorizon4`) and launch the game. Please remember to **disable the Steam controller mapping**.
   - b. You can adjust the vibration intensity by right-clicking on `X1nput.ini`, selecting "Open with" > "Notepad" (or any text editor), and modifying the strength values.
   - c. Refer to the included image to enable game output in the game settings (use your computer's IP address, and set DATA OUT IP PORT to 9999).

2. **Advanced Usage**:
   - a. If you want to modify detailed parameters, please download and install Visual Studio with C++ desktop development, and open the `.sln` file. You will find `dllmain.cpp`; feel free to make your modifications and consider open-sourcing your changes.
   - b. After running `dllmain.cpp`, a `.dll` file will be generated. Again, place `XInput1_3.dll` and `X1nput.ini` into the game folder (usually located at `C:\Program Files (x86)\Steam\steamapps\common\ForzaHorizon4`) and launch the game. Please remember to **disable the Steam controller mapping**.
   - c. Adjust the motor vibration levels by right-clicking on `X1nput.ini`, selecting "Open with" > "Notepad" (or any text editor), and changing the strength values.
   - d. Refer to the included image to enable game output in the game settings (use your computer's IP address, and set DATA OUT IP PORT to 9999).

## For detailed decoding of game output content, please refer to [this link].

---

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information, see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

-----------

# 軟體說明

這個專案由兩個主要部分組成：
1. 讀取遊戲的輸出並進行處理以獲得遊戲統計數據。
2. 使用 X1nput 將定義的統計數據應用於控制器的觸發器。以下是詳細說明：

1. 讀取遊戲的輸出並計算遊戲統計數據，具有以下功能：
   - a. 根據轉速控制右側震動觸發器（隨著正規化轉速的增加，震動強度也會增強，產生強烈的反饋感，這是通過指數函數實現的）。
   - b. 當啟動 ABS 時控制左側震動觸發器（通過監控四個輪子的滑移平方和的平方根來實現）。
   - c. 碰撞時產生震動（監控加速度變化；如果任何方向的加速度值過高，則觸發"BUMP"震動）。
   - d. 倒退檔時提供震動提醒（監控檔位位置）。
   - e. 確保遊戲暫停時觸發器不會震動（如果引擎轉速為 0，則暫停震動。感謝 [richstokes](https://github.com/richstokes/Forza-data-tools) 的靈感）。

2. 利用 X1nput 將定義的統計數據應用於控制器的觸發器，這是根據 [araghon007](https://github.com/araghon007/X1nput) 的修改。以下是他的原文：
   *X1nput 是使用 Windows.Gaming.Input API 重新實現的 Xinput，該 API 對 Xbox One 控制器提供了更好的支持，包括衝擊觸發器。*

## 安裝

1. **簡單使用**：
   - a. 如果您希望使用預設參數，請將 `XInput1_3.dll` 和 `X1nput.ini` 複製到遊戲資料夾中（通常位於 `C:\Program Files (x86)\Steam\steamapps\common\ForzaHorizon4`）並啟動遊戲。請記得**關閉 Steam 控制器映射**。
   - b. 您可以通過右鍵單擊 `X1nput.ini`，選擇 "以記事本打開"（或任何文本編輯器），來調整震動強度。
   - c. 請參考附帶的圖片，將遊戲中的遊戲輸出打開（使用您電腦的 IP 地址，並將 DATA OUT IP PORT 設定為 9999）。

2. **進階使用**：
   - a. 如果您想修改詳細參數，請下載並安裝 Visual Studio 並使用 C++ 桌面開發，打開 `.sln` 文件。您將找到 `dllmain.cpp`，隨意進行修改，並歡迎您將其開源。
   - b. 在運行 `dllmain.cpp` 後，將生成一個 `.dll` 文件。再次，將 `XInput1_3.dll` 和 `X1nput.ini` 放入遊戲資料夾中（通常位於 `C:\Program Files (x86)\Steam\steamapps\common\ForzaHorizon4`）並啟動遊戲。請記得**關閉 Steam 控制器映射**。
   - c. 透過右鍵單擊 `X1nput.ini`，選擇 "以記事本打開"（或任何文本編輯器），來調整馬達震動的程度。
   - d. 請參考附帶的圖片，將遊戲中的遊戲輸出打開（使用您電腦的 IP 地址，並將 DATA OUT IP PORT 設定為 9999）。

## 有關遊戲輸出內容解碼的詳細信息，請參閱[https://github.com/DANIEL6509/Forza-Horizon-C-OUTPUT]。

![My Image1](https://github.com/DANIEL6509/POWERFUL_X1nput/blob/main/STEP_1.png?raw=true)
![My Image1](https://github.com/DANIEL6509/POWERFUL_X1nput/blob/main/STEP_2.png?raw=true)
![My Image1](https://github.com/DANIEL6509/POWERFUL_X1nput/blob/main/STEP_3.png?raw=true)


---

該專案已採納 [Microsoft 開源行為準則](https://opensource.microsoft.com/codeofconduct/)。如需更多信息，請參見 [行為準則常見問題解答](https://opensource.microsoft.com/codeofconduct/faq/)，或通過電子郵件 [opencode@microsoft.com](mailto:opencode@microsoft.com) 聯繫我們以獲取其他問題或建議。
