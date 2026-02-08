# Sound Recorder (WinAPI)

**Главные правила проекта:** минимальный размер, использование стандартных библиотек, максимальная совместимость.

## Совместимость
Программа стабильно работает на всех версиях Windows, начиная с **Windows XP**. 
*Протестировано в эмуляторе и на реальных сборках XP.*

## Основные возможности
1. **Запись:** захват аудио с выбранного устройства записи.
2. **Воспроизведение:** прослушивание записанного материала.
3. **Работа с WAV:** сохранение и загрузка аудиоданных в формате WAV PCM (.wav).
4. **Гибкая настройка:** выбор устройства записи, глубины звука, частоты дискретизации и режима (Моно/Стерео).
5. **Real-time Визуализация:** отображение звуковой волны во время записи и воспроизведения.
6. **Контроль уровней:** индикаторы громкости (VU meter) и перегрузки (Peak meter).
7. **Waveform:** отображение полного графика записанного файла.
8. **Редактирование:** обрезка выделенного фрагмента записи.
9. **Громкость:** изменение уровня громкости записанного звука.

## Технологии
* **Язык:** C++ (ISO C++14/17).
* **Библиотеки звука:** WinMM (Windows Multimedia API) — `waveIn`/`waveOut`.
* **Графика:**
    * **GDI:** Стандартный нативный интерфейс Windows (кнопки, меню, списки, диалоги).
    * **OpenGL:** Аппаратно-ускоренный рендеринг для графика звука, VU meter, Peak meter.
* **Сборка:** CMake.

## Как собрать проект
Для сборки под Windows XP рекомендуется использовать **Visual Studio 2017/2019/2022** с установленным компонентом `v141_xp`.

1. Запустите `GenerateProject.bat` для генерации файлов решения.
2. Откройте `sound_rec.sln` в папке `build`.
3. Соберите проект (Сборка -> Собрать решение или нажмите F5).

## Версии и история изменений
* Актуальную версию программы всегда можно увидеть в окне **"О программе"** (определено в файле `version.h`).
* Список всех изменений фиксируется в файле [changelog.txt](./changelog.txt)

---

# Sound Recorder (WinAPI) [EN]

**Core principles:** minimal size, native libraries only, maximum compatibility.

## Compatibility
The application runs stably on all Windows versions starting from **Windows XP**.
*Tested on emulators and physical Windows XP hardware.*

## Features
1. **Recording:** Capture audio from the selected input device.
2. **Playback:** Listen to the recorded audio data.
3. **WAV Support:** Save and load audio files in WAV PCM (.wav) format.
4. **Flexible Settings:** Select recording device, bit depth, sample rate, and mode (Mono/Stereo).
5. **Real-time Visualization:** Waveform rendering during recording and playback.
6. **Levels Monitoring:** Volume indicators (VU meter) and clipping alerts (Peak meter).
7. **Full Waveform:** Display the entire graph of the recorded file.
8. **Editing:** Trim selected audio fragments.
9. **Volume Control:** Adjust the volume level of the recorded sound.

## Technologies
* **Language:** C++ (ISO C++14/17).
* **Audio API:** WinMM (Windows Multimedia API) — `waveIn`/`waveOut`.
* **Graphics:**
    * **GDI:** Native Windows interface (buttons, menus, lists, dialogs).
    * **OpenGL:** Hardware-accelerated rendering for waveform and meters.
* **Build System:** CMake.

## How to Build
To build for Windows XP, we recommend using **Visual Studio 2017/2019/2022** with the `v141_xp` toolset installed.

1. Run `GenerateProject.bat` to generate the solution files.
2. Open `sound_rec.sln` inside the `build` folder.
3. Build the project (Build -> Build Solution).

## Versions & Changelog
* The current application version is available in the **"About"** window (defined in `version.h`).
* Detailed history is available in the [changelog.txt](./changelog.txt) file.