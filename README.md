# Sound Recorder (WinAPI) [RU]

## Главные правила проекта
* **Минимализм:** Исполняемый файл занимает минимум места.
* **Стандартные библиотеки:** Использование чистого WinAPI и STL.
* **Максимальная совместимость:** Стабильная работа от **Windows XP** до Windows 11.

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

## Технологии и Архитектура
* **Язык:** C++ (ISO C++14/17).
* **Многопоточность (Multithreading):** * Раздельные потоки для записи (`std::thread`) и воспроизведения для исключения задержек (low-latency).
    * Потокобезопасная обработка буферов аудиоданных.
* **Библиотеки звука:** WinMM (Windows Multimedia API) — прямое взаимодействие с драйверами через `waveIn`/`waveOut`.
* **Графика и UI:**
    * **GDI:** Нативный интерфейс Windows (меню, диалоги) — обеспечивает минимальный вес и мгновенный отклик.
    * **OpenGL:** Аппаратное ускоренние для графика звука, VU meter, Peak meter.
* **Сборка:** CMake.

## Как собрать проект
Для сборки под Windows XP рекомендуется использовать **Visual Studio 2017/2019/2022** с установленным компонентом `v141_xp`.

1. Запустите `GenerateProject.bat` для генерации файлов решения.
2. Откройте `sound_rec.sln` в папке `build`.
3. Соберите проект ("Сборка -> Собрать решение" или нажмите F5).

## Версии и история изменений
* Актуальную версию программы всегда можно увидеть в окне **"О программе"** (определено в файле `version.h`).
* Список всех изменений фиксируется в файле [changelog.txt](./changelog.txt)

---

# Sound Recorder (WinAPI) [EN]

## Core Principles
* **Minimalism:** Tiny executable size with no heavy dependencies.
* **Standard Libraries:** Built using pure WinAPI and C++ Standard Template Library (STL).
* **Maximum Compatibility:** Solid performance on all Windows versions, from **Windows XP** to Windows 11.

## Key Features
1. **Recording:** High-quality audio capture from any selected input device.
2. **Playback:** Instant monitoring and playback of recorded material.
3. **WAV Processing:** Full support for saving and loading WAV PCM (.wav) files.
4. **Flexible Configuration:** Adjustable bit depth, sample rate, and Mono/Stereo modes.
5. **Real-time Visualization:** Live waveform rendering during recording and playback.
6. **Level Control:** High-precision VU meters and Peak meters (clipping indicators).
7. **Full Waveform View:** Visual representation of the entire recorded audio file.
8. **Editing:** Basic trimming functionality for selected audio fragments.
9. **Volume Control:** Digital gain adjustment for recorded audio.

## Tech Stack & Architecture
* **Language:** C++ (ISO C++14/17).
* **Multithreading:** * Dedicated threads for recording and playback (`std::thread`) to ensure **low-latency** performance.
    * Thread-safe audio buffer management.
* **Audio API:** WinMM (Windows Multimedia API) — direct driver interaction via `waveIn`/`waveOut`.
* **Graphics & UI:**
    * **GDI:** Native Windows interface (menus, dialogs) for a lightweight feel and instant response.
    * **OpenGL:** Hardware-accelerated rendering for smooth waveform and meter animations.
* **Build System:** CMake.

## How to Build
To build for Windows XP, it is recommended to use **Visual Studio 2017/2019/2022** with the `v141_xp` platform toolset installed.

1. Run `GenerateProject.bat` to generate the solution files.
2. Open `sound_rec.sln` located in the `build` folder.
3. Build the project ("Build -> Build Solution" or press F5).

## Versions & Changelog
* The current application version is available in the **"About"** window (defined in `version.h`).
* Detailed history is available in the [changelog.txt](./changelog.txt) file.