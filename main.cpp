#include "stdafx.h"
#include "resource.h"
#include "my_res.h"
#include "version.h" // храним версию программы
#include "dpi_utils.h" // функции для работы с DPI окна/системы

// стили курсора мышки
static HCURSOR hCursorHand = NULL;
static HCURSOR hCursorArrow = NULL;
static HCURSOR hCursorWait = NULL;

// стили шрифта различных элементов окна
HFONT hFontButton = NULL; // стиль шрифта кнопок
HFONT hFontComboBox = NULL; // стиль шрифта для GroupBox и ComboBox
HFONT hFonthList1 = NULL; // стиль шрифта для hList1 (немного мельче и тоньше)

// хэндлы!
HINSTANCE hInst;
HWND hWnd;

// храним размеры главного окна
int wW = 770; // MIN SIZE - 
int wH = 570; // MIN SIZE - 

// храним DPI главного окна
UINT WindowDPI = 0; // (равен нулю до первой инициализации)

// кисть для зарисовки цвета фона окна и фона для hGroupBox внутри окна
HBRUSH WhiteColorBrush = NULL; // белая кисть, инициализируем в main()

// диапазон внутри области рисования OpenGL
double glXmin = 0;
double glXmax = 1;
double glYmin = 0;
double glYmax = 1;
// положение области рисования OpenGL относительно главного окна
int glViewportX, glViewportY, glViewportWidth, glViewportHeight;

// флаги для OpenGL
BOOL ShowAllSound = FALSE; // отображаем весь аудио файл в порте просмотра? (можно избавиться)

// упаравляющие элементы окна
HWND hList1; // список вместо коммандной строки
HWND hComboBox1; // для выбора глубины звука
HWND hComboBox2; // для выбора частоты дискретизации
HWND hComboBox3; // для выбора СТЕРЕО или МОНО
HWND hComboBox5; // для выбора устройства записи
HWND hBut1;// кнопка Rec
HWND hBut2;// кнопка Stop
HWND hBut3;// кнопка Play
HWND hBut4;// кнопка Очистки list'a
HWND hBut5; // кнопка остановки воспроизведения
HWND hButPPause; // кнопка приостановки воспроизведения
HWND hButCutSound; // кнопка для обрезки звука
HWND hGroupBox2; // рамка "Устройство записи"
HWND hGroupBox3; // рамка "Устройство воспроизведения"
HWND hTrBar1; // трекбар начала выделения
HWND hTrBar2; // трекбар бар конца выделения фрагмента звука
HWND hWndTime; // STATIC, куда выводим в виде текста время воспроизведения, записи или длительность аудио
HWND hCheckBoxNoSave; // чекбокс, 1 == визуализируем входящий поток, но не сохраняем в Head_rec
HWND hWndRecSymbol; // STATIC, выводим красным цветом надпись REC обозначающую процесс записи
HWND hCheckBoxShowMessageLog; // вкл/выкл отображение hList1 (окно диагн. сообщений)

// Хэндлы OpenGL
HGLRC hGLRC = NULL;
HDC hDC = NULL;

// данные для буферов записи и чтения
WORD myChannels = 1; // моно
DWORD mySamplesPerSec = 44100; // 44.1 килогерц/секунду
WORD myBitsPerSample = 16;// 16 бит глубина звучания

// список в котором храним записанный фрагмент
struct rec_list {
	char* data;
	rec_list* next;
} *Head_rec;

// структура, в которой храним график всего звука в сжатом виде
struct soundGraph{
	bool Ready; // построен ли массив
	bool Show; // будем ли визуализировать массив
	ULONG Length; // размерность массивов DataL[Length][2] и DataR[Length][2]
	double (*DataL)[2]; // левый канал
	double (*DataR)[2]; // правый канал
} mySoundGraph;

// обьявления для устройства записи
UINT waveInDevNummer; // номер устройства записи, которое используем (по умолчанию первое в списке)
HWAVEIN hRec_device = NULL; // ссылка на устройство записи
WAVEFORMATEX struct_rec_data; // формат записываемого звука
WAVEHDR waveHdr[2]; // заголовки буфера записи
BOOL AktivRecBufer = 0;// в какой буфер записался (для MM_WIM_DATA)
ULONG BufferSize = 8000; // размер буфера записи/воспр. в байтах (по умолчанию 50мс аудиоданных)
double RecBufferLenght; // длина записанных аудиоданных в байтах
double AudioTimeMS; // длина записаного аудио в миллисекундах
BOOL DetailedShow = TRUE; // визуализация каждой (через одну == FALSE) точки текущего буфера

// объявления для устройства воспроизведения
HWAVEOUT hPlay_device = NULL; // ссылка на устройство воспроизведения
WAVEFORMATEX struct_play_data; // формат воспроизводимого звука
WAVEHDR waveHdrPlay[2]; // заголовки буфера проигрывания
BOOL AktivPlayBufer = 0;// какой буфер проигрался (для MM_WOM_DONE) 
bool isNowPause = false; // включена ли пауза при проигрывании
double PlayBufferPos = 0; // сколько уже фрагмента проиграно (всего RecBufferLenght данных)
double PlayTimeMS = 0; // таймер воспроизведения в миллисекундах


// визуализация перегрузки во время записи/воспроизведения
float LeftOverload70 = 0; // отображение перегрузки 70% ЛК
float LeftOverload90 = 0; // отображение перегрузки 90% ЛК
float LeftOverload99 = 0; // отображение перегрузки 99% ЛК
float RightOverload70 = 0; // отображение перегрузки 70% ПК
float RightOverload90 = 0; // отображение перегрузки 90% ПК
float RightOverload99 = 0; // отображение перегрузки 99% ПК
float OverloadDelay = 5; // присваиваем это значение перегрузке при её обнаружении
BOOL ShowOverload = TRUE; // будем ли визуализировать перегрузку звука

// визуализация VU meter
BOOL ShowVUMeter = TRUE; // будем ли визуализировать VU meter

// визуализация пиков (перегрузки) при визуализиции всего аудио
BOOL showPeaks = TRUE;


// переменные для отдельного потока записи
// Ссылка на объект потока записи (он создаётся и завершается по кнопкам)
static std::thread recThread;
// Флаг, по которому поток записи узнаёт, что нужно остановиться
static std::atomic<bool> RecStopFlag{ true };
// запущен ли сейчас поток записи
static std::atomic<bool> isRecThreadRunning{ false };
// флаг, возвращаемый в главный поток, отображает насколько успешно всё записано
std::atomic<int> RecResult{ 0 }; // >0 = ошибка при попытке воспр, 0 = всё успешно
// защита RecBufferLenght и AudioTimeMS при совместном использовании в разных потоках
std::mutex RecBufferLenght_Mutex;
// храним текст с описанием всех ошибок, которые возникли во время записи
WCHAR RecMsgBoxErrString[500];
// храним ссылку на последний фрагмент для передачи в главный поток
rec_list *Rec_Last_rec = NULL;
// защита Rec_Last_rec при совместном использовании в разных потоках
std::mutex Last_rec_Mutex;
// визуализируем поток записи с выбранного устройства без сохранения 
static std::atomic<bool> RecNoSave{ false };

// переменные для отдельного потока воспроизведения
// Ссылка на объект потока воспроизведения (он создаётся и завершается по кнопкам)
static std::thread playThread;
// Флаг, по которому поток воспроизведения узнаёт, что нужно остановиться
static std::atomic<bool> PlayStopFlag{ true };
// запущен ли сейчас поток воспроизведения
static std::atomic<bool> isPlayThreadRunning{ false };
// флаг, возвращаемый в главный поток, отображает насколько успешно всё воспроизведено
std::atomic<int> PlayResult{ 0 }; // >0 = ошибка при попытке воспр, 0 = всё успешно
// защита PlayBufferPos и PlayTimeMS при совместном использовании в разных потоках
std::mutex PlayBufferPos_Mutex;
// храним текст с описанием всех ошибок, которые возникли во время воспроизведения
WCHAR PlayMsgBoxErrString[350];
// храним ссылку на текущий воспроизводимый фрагмент для передачи в главный поток
std::atomic<rec_list*> Play_Last_rec{ nullptr };

// прототиты функций
void reshape(int x, int y, int width, int height);
BOOL WaveRead(WCHAR* cFileName);
BOOL WaveSave(WCHAR* cFileName,rec_list *Head, unsigned long lDataSize, unsigned long lBufSize, WAVEFORMATEX WaveInfo);
BOOL CutSound(rec_list *Head, unsigned long uStartBufPos, unsigned long uKolBuffers);// обрезать звук с выбранного буфера до +количество буферов
void DrawHorizontalLine(double XStart, double XEnd, double Y, double Width_px); // рисуем горизонтальную линию толщиной Width_px пикселей
void DrawQuadPoint(double X_gl, double Y_gl, double Size_px); // рисуем точку в виде квадрата с размером Size_px пикселей
void display(double localPlayBufferPos = 0); // функция для отрисовки области OpenGL
void ShowAllBufStereo(BOOL ShowPeaks); // отобразить всю дорожку звуковую если она стерео
void ShowLastBufStereo(rec_list *ShowingBuffer, BOOL ShowAllPoints); // отобразить стерео-буфер как 2 дорожки
void ShowLastBufStereoOverload(rec_list *ShowingBuffer); // отобразить стерео-буфер как 2 дорожки (плюс перегрузка)
void ShowPeakMeterStereo(rec_list* ShowingBuffer); // отображение Peak Meter для стерео-буфера
void ShowVUMeterStereo(rec_list* ShowingBuffer); // отображение VU Meter для стерео-буфера
void ShowAllBufMono(BOOL ShowPeaks); // отобразить всю звуковую дорожку если она моно
void ShowLastBufMono(rec_list *ShowingBuffer, BOOL ShowAllPoints);// отобразить моно-буфер
void ShowLastBufMonoOverload(rec_list *ShowingBuffer); // отобразить моно-буфер (плюс перегрузка)
void ShowPeakMeterMono(rec_list* ShowingBuffer); // отображение Peak Meter для моно-буфера
void ShowVUMeterMono(rec_list* ShowingBuffer); // отображение VU Meter для моно-буфера
bool ChVolume(rec_list* Head, ULONG HeadSize, double volumeMultiplier); // изменяем громкость звука
void Clipping(rec_list* Head, ULONG HeadSize, double  ClippingMultiplier); // Обрезаем уровень звука выше заданного порога
double convertSampleToDouble(const uint8_t* sampleBytes); //  возвращает double значение, полученое из 24 бит (3 байта) звука
void RedrawGroupBoxBackground(HWND hwndGroupBox, HBRUSH hBachgroundBrush);// перерисовывает фон GroupBox цветом окна родителя
BOOL ListBoxAddStr(HWND hLB, LPCWSTR AddStr, UINT MaxLength = 25); // добавляет элемент в конец ListBox, контролируя его размер
void RedrawWindowElements(int W, int H, float Scale); // обновляет положение элементов внутри окна при изм. размеров
rec_list* GetAudioPosition(rec_list*, ULONG, double, double, double*); // возвращает буфер по переданному положению(от 0 до 1) в аудио
BOOL ShowAudioTime(HWND hWndOutput, double AudioDurationMS, double AudioCurrentPosMS = 0); // отображаем время записи/воспроизведения для текущего аудио
INT_PTR CALLBACK VolumeDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam); // функция обработки сообщений диалогового окона изм. громкости

void PrepareSoundGraph(rec_list* Head, ULONG BufferSize, double RecBufferLenght); // вычислить график всего звука (в переменную mySoundGraph) имея звуковые данные (Head ...)
void ShowSoundGraph(BOOL ShowPeaks); // отобразить график всего звука (переменная mySoundGraph)

void CALLBACK waveRecProc(HWAVEIN hRecDev,UINT uMsg,DWORD_PTR dwInstance,\
	DWORD_PTR dwParam1,DWORD_PTR dwParam2); // функция обработчик событий при записи звука
void RecordingThreadFunc(); // отдельный поток записи звука

void CALLBACK wavePlayProc(HWAVEOUT hPlayDev, UINT uMsg, DWORD_PTR dwInstance, \
	DWORD_PTR dwParam1, DWORD_PTR dwParam2); // функция обработчик событий при воспр. звука
void PlayingThreadFunc(); // отдельный поток воспроизведения звука


//  --- реализация функций ---

// Callback-функция для waveOut (поток воспроизведения)
void CALLBACK wavePlayProc(HWAVEOUT hPlayDev, UINT uMsg, DWORD_PTR dwInstance, \
	DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	switch (uMsg)
	{
	case WOM_DONE: // проигрался очередной блок с очереди воспроизведения
	{
		if (PlayStopFlag.load() == false) // пока флаг остановки разрешает воспроизведение
		{
			UINT uReturnResult; // буферная переменная

			rec_list* Local_Last_rec = Play_Last_rec.load(std::memory_order_acquire); // для упрощения работы

			// пока позиция воспроизведения не дошла до конца файла
			if (Local_Last_rec->next != NULL)
			{
				// переходим к следующей записи
				Local_Last_rec = Local_Last_rec->next;

				waveHdrPlay[AktivPlayBufer].lpData = Local_Last_rec->data;

				uReturnResult = waveOutWrite(hPlay_device, &waveHdrPlay[AktivPlayBufer], sizeof(WAVEHDR));

				if (uReturnResult != MMSYSERR_NOERROR)
				{
					WCHAR s[10];
					wcscat(PlayMsgBoxErrString, L"\n - функция waveOutWrite(), код ошибки - ");
					_itow((int)uReturnResult, s, 10);
					wcscat(PlayMsgBoxErrString, s); // текст ошибки в потоке воспр.

					PlayResult.store(1); // флаг ошибки в потоке воспроизведения

					PlayStopFlag.store(true); // флаг остановки воспроизведения
				}

				else
				{
					PlayBufferPos_Mutex.lock(); // блокируем PlayBufferPos и PlayTimeMS в других потоках

					// сохраняем в атомарную переменную текущую позицию воспроизведения (для визуализации)
					Play_Last_rec.store(Local_Last_rec, std::memory_order_release);

					// позиция воспроизведения и время воспроизведения увеличились
					::PlayBufferPos += ::BufferSize; // проигран очередной фрагмент
					::PlayTimeMS = ::PlayBufferPos / ::mySamplesPerSec / (::myBitsPerSample / 8) / ::myChannels * 1000;

					PlayBufferPos_Mutex.unlock(); // разблокируем PlayBufferPos и PlayTimeMS в других потоках

					PostMessage(hWnd, WM_USER + 1, 0, 0); // перерисовываем главное окно
				}

				AktivPlayBufer = !AktivPlayBufer;

			} // if (Local_Last_rec->next != NULL)

			// конец файла - завершаем безопасно воспроизведение
			else
			{
				PlayStopFlag.store(true); // флаг остановки
			}

		} // пока флаг остановки разрешает воспроизведение

	} // WOM_DONE
	break;

	default:
		break;
	}

};

// Функция потока воспроизведения: запускает воспроизведение
void PlayingThreadFunc()
{
	static WCHAR s[250]; // буферные строки
	// static WCHAR s1[300]; // буферные строки

	PlayBufferPos_Mutex.lock(); // блокируем PlayBufferPos и PlayTimeMS в других потоках
	::PlayBufferPos = 0; // позиция проигрывания устанавливается в начало
	::PlayTimeMS = 0; // таймер проигрывания звука
	PlayBufferPos_Mutex.unlock(); // разблокируем PlayBufferPos и PlayTimeMS в других потоках

	::AktivPlayBufer = 0; // запись в буфер воспр. начинается с нулевого

	// позиция воспроизведения на начало аудиоданных
	Play_Last_rec.store(Head_rec, std::memory_order_release);

	// Пытаемся открыть устр. воспроизведения

	PlayResult.store(0); // по умолчанию у нас успех во время воспроизведения
	wcscpy(PlayMsgBoxErrString, L"Во время воспроизведения возникли ошибки:\n"); // первая строка, если будут ошибки

	UINT ReturnResult; // временно храним результат, возвращаемый функциями waveOut...

	// пытаемся открыть устройство воспр.
	ReturnResult = waveOutOpen(&hPlay_device, WAVE_MAPPER,
		&struct_play_data, (DWORD_PTR)wavePlayProc, NULL, CALLBACK_FUNCTION);

	// Устройство воспроизведения успешно открыто
	if (ReturnResult == MMSYSERR_NOERROR)
	{
		// буфер 1
		waveHdrPlay[0].dwBufferLength = ::BufferSize; // размер буфера данных
		waveHdrPlay[0].lpData = Play_Last_rec.load(std::memory_order_acquire)->data;
		waveHdrPlay[0].dwFlags = NULL; // флаги потом система установит сама

		ReturnResult = waveOutPrepareHeader(hPlay_device, &waveHdrPlay[0], sizeof(WAVEHDR));// готовим буфер

		if (ReturnResult != MMSYSERR_NOERROR)
		{
			if (hPlay_device)
			{
				waveOutClose(hPlay_device);
				hPlay_device = NULL;
			}

			wcscat(PlayMsgBoxErrString, L"\n - функция waveOutPrepareHeader(1), код ошибки - ");
			_itow((int)ReturnResult, s, 10);
			wcscat(PlayMsgBoxErrString, s); // текст ошибки в потоке воспр.

			PlayResult.store(1); // флаг ошибки в потоке воспроизведения

			PlayStopFlag.store(true); // флаг остановки воспроизведения

			// освобождаем ресурсы playThread после остановки этого потока
			// перерисовываем кнопки
			// меняем область OpenGL (рисуем весь график звука) и перерисовываем главное окно 
			PostMessage(hWnd, WM_USER + 4, 0, 0);

			return;

		}

		// буфер 2
		Play_Last_rec.store(Play_Last_rec.load(std::memory_order_acquire)->next, \
			std::memory_order_release);// переход к след элементу списка воспроизведения

		waveHdrPlay[1].dwBufferLength = ::BufferSize; // размер буфера данных
		waveHdrPlay[1].lpData = Play_Last_rec.load(std::memory_order_acquire)->data;
		waveHdrPlay[1].dwFlags = NULL; // флаги потом система установит сама

		ReturnResult = waveOutPrepareHeader(hPlay_device, &waveHdrPlay[1], sizeof(WAVEHDR));// готовим заголовок буфера

		if (ReturnResult != MMSYSERR_NOERROR)
		{
			waveOutUnprepareHeader(hPlay_device, &waveHdrPlay[0], sizeof(WAVEHDR));

			if (hPlay_device)
			{
				waveOutClose(hPlay_device);
				hPlay_device = NULL;
			}

			wcscat(PlayMsgBoxErrString, L"\n - функция waveOutPrepareHeader(2), код ошибки - ");
			_itow((int)ReturnResult, s, 10);
			wcscat(PlayMsgBoxErrString, s); // текст ошибки в потоке воспр.

			PlayResult.store(1); // флаг ошибки в потоке воспроизведения

			PlayStopFlag.store(true); // флаг остановки воспроизведения

			// освобождаем ресурсы playThread после остановки этого потока
			// перерисовываем кнопки
			// меняем область OpenGL (рисуем весь график звука) и перерисовываем главное окно 
			PostMessage(hWnd, WM_USER + 4, 0, 0);

			return;
		}

		// буфер 1 в очередь воспр.
		ReturnResult = waveOutWrite(hPlay_device, &waveHdrPlay[0], sizeof(WAVEHDR));

		if (ReturnResult != MMSYSERR_NOERROR)
		{
			waveOutUnprepareHeader(hPlay_device, &waveHdrPlay[0], sizeof(WAVEHDR));

			waveOutUnprepareHeader(hPlay_device, &waveHdrPlay[1], sizeof(WAVEHDR));

			if (hPlay_device)
			{
				waveOutClose(hPlay_device);
				hPlay_device = NULL;
			}

			wcscat(PlayMsgBoxErrString, L"\n - функция waveOutWrite(1), код ошибки - ");
			_itow((int)ReturnResult, s, 10);
			wcscat(PlayMsgBoxErrString, s); // текст ошибки в потоке воспр.

			PlayResult.store(1); // флаг ошибки в потоке воспроизведения

			PlayStopFlag.store(true); // флаг остановки воспроизведения

			// освобождаем ресурсы playThread после остановки этого потока
			// перерисовываем кнопки
			// меняем область OpenGL (рисуем весь график звука) и перерисовываем главное окно 
			PostMessage(hWnd, WM_USER + 4, 0, 0);

			return;
		}
		else
		{
			PlayBufferPos_Mutex.lock(); // блокируем PlayBufferPos и PlayTimeMS в других потоках

			// позиция воспроизведения и время воспроизведения увеличились
			::PlayBufferPos += ::BufferSize; // проигран очередной фрагмент
			::PlayTimeMS = ::PlayBufferPos / ::mySamplesPerSec / (::myBitsPerSample / 8) / ::myChannels * 1000;

			PlayBufferPos_Mutex.unlock(); // разблокируем PlayBufferPos и PlayTimeMS в других потоках
		}

		// буфер 2 в очередь воспр.
		ReturnResult = waveOutWrite(hPlay_device, &waveHdrPlay[1], sizeof(WAVEHDR));

		if (ReturnResult != MMSYSERR_NOERROR)
		{
			waveOutReset(hPlay_device); // освобождаем буфер 1  из драйвера

			PlayBufferPos_Mutex.lock(); // блокируем PlayBufferPos и PlayTimeMS в других потоках

			// позиция воспроизведения и время воспроизведения обнулились
			::PlayBufferPos = 0;
			::PlayTimeMS = 0;

			PlayBufferPos_Mutex.unlock(); // разблокируем PlayBufferPos и PlayTimeMS в других потоках


			waveOutUnprepareHeader(hPlay_device, &waveHdrPlay[0], sizeof(WAVEHDR));

			waveOutUnprepareHeader(hPlay_device, &waveHdrPlay[1], sizeof(WAVEHDR));

			if (hPlay_device)
			{
				waveOutClose(hPlay_device);
				hPlay_device = NULL;
			}

			wcscat(PlayMsgBoxErrString, L"\n - функция waveOutWrite(2), код ошибки - ");
			_itow((int)ReturnResult, s, 10);
			wcscat(PlayMsgBoxErrString, s); // текст ошибки в потоке воспр.

			PlayResult.store(1); // флаг ошибки в потоке воспроизведения

			PlayStopFlag.store(true); // флаг остановки воспроизведения

			// освобождаем ресурсы playThread после остановки этого потока
			// перерисовываем кнопки
			// меняем область OpenGL (рисуем весь график звука) и перерисовываем главное окно 
			PostMessage(hWnd, WM_USER + 4, 0, 0);

			return;

		}
		else
		{
			PlayBufferPos_Mutex.lock(); // блокируем PlayBufferPos и PlayTimeMS в других потоках

			// позиция воспроизведения и время воспроизведения увеличились
			::PlayBufferPos += ::BufferSize; // проигран очередной фрагмент
			::PlayTimeMS = ::PlayBufferPos / ::mySamplesPerSec / (::myBitsPerSample / 8) / ::myChannels * 1000;

			PlayBufferPos_Mutex.unlock(); // разблокируем PlayBufferPos и PlayTimeMS в других потоках

		}

		// если запуск воспроизведения прошёл успешно
		PostMessage(hWnd, WM_USER + 5, 0, 0); // меняем состояние кнопок ПАУЗА и СТОП ВОСПР.

	} // Устройство воспроизведения успешно открыто

	// невозможно открыть устройство воспроизведения 
	else
	{
		// пишем текст ошибки для вывода в PlayMsgBoxErrString
		wcscat(PlayMsgBoxErrString, L"\n- функция waveOutOpen(), код ошибки - ");
		_itow((int)ReturnResult, s, 10);
		wcscat(PlayMsgBoxErrString, s);

		// пытаемся получить текст ошибки на английском для вывода в PlayMsgBoxErrString
		if (waveOutGetErrorTextW(ReturnResult, s, 200) == MMSYSERR_NOERROR)
		{
			wcscat(PlayMsgBoxErrString, L"\n(");
			wcscat(PlayMsgBoxErrString, s);
			wcscat(PlayMsgBoxErrString, L")");
		}

		waveOutClose(hPlay_device); // на всякий случай
		hPlay_device = NULL;

		PlayResult.store(1); // флаг ошибки в потоке воспроизведения

		PlayStopFlag.store(true); // включаем флаг остановки

		// освобождаем ресурсы playThread после остановки этого потока
		// перерисовываем кнопки
		// меняем область OpenGL (рисуем весь график звука) и перерисовываем главное окно 
		PostMessage(hWnd, WM_USER + 4, 0, 0);

		return;

	}

	// если не было ошибок при запуске воспроизведения
	if (PlayResult.load() == 0)
	{
		// поток работает, пока флаг остановки PlayStopFlag равен false  
		// (проверка каждые 10 мс)
		while (PlayStopFlag.load() != true)
		{
			Sleep(10);
		}
	}

	// Получили команду через флаг PlayStopFlag,
	// останавливаем и очищаем всё, что связано с waveOut !!!
	
	// завершаем безопасно воспроизведение
	ReturnResult = waveOutReset(hPlay_device);

	if (ReturnResult != MMSYSERR_NOERROR)
	{
		// ошибка при вызове waveOutReset()
		// пишем текст ошибки для вывода в PlayMsgBoxErrString
		wcscat(PlayMsgBoxErrString, L"\n - функция waveOutReset(), код ошибки - ");
		_itow((int)ReturnResult, s, 10);
		wcscat(PlayMsgBoxErrString, s);

		PlayResult.store(1); // флаг ошибки в потоке воспроизведения
	}

	PlayBufferPos_Mutex.lock(); // блокируем PlayBufferPos и PlayTimeMS в других потоках
	::PlayBufferPos = 0; // позиция проигрывания устанавливается в начало
	::PlayTimeMS = 0; // таймер проигрывания звука
	PlayBufferPos_Mutex.unlock(); // разблокируем PlayBufferPos и PlayTimeMS в других потоках

	// освобождаем буферы воспроизведения
	ReturnResult = waveOutUnprepareHeader(hPlay_device, &waveHdrPlay[0], sizeof(WAVEHDR));

	if (ReturnResult != MMSYSERR_NOERROR)
	{
		// пишем текст ошибки для вывода в PlayMsgBoxErrString
		wcscat(PlayMsgBoxErrString, L"\n - функция waveOutUnprepareHeader(1), код ошибки - ");
		_itow((int)ReturnResult, s, 10);
		wcscat(PlayMsgBoxErrString, s);

		PlayResult.store(1); // флаг ошибки в потоке воспроизведения
	}

	ReturnResult = waveOutUnprepareHeader(hPlay_device, &waveHdrPlay[1], sizeof(WAVEHDR));

	if (ReturnResult != MMSYSERR_NOERROR)
	{
		// пишем текст ошибки для вывода в PlayMsgBoxErrString
		wcscat(PlayMsgBoxErrString, L"\n - функция waveOutUnprepareHeader(2), код ошибки - ");
		_itow((int)ReturnResult, s, 10);
		wcscat(PlayMsgBoxErrString, s);

		PlayResult.store(1); // флаг ошибки в потоке воспроизведения
	}

	// пытаемся закрыть устройство воспроизведения
	ReturnResult = waveOutClose(hPlay_device); // если можно то закрываем устройство воспроизведения

	if (ReturnResult != MMSYSERR_NOERROR)
	{
		// пишем текст ошибки для вывода в PlayMsgBoxErrString
		wcscat(PlayMsgBoxErrString, L"\n - функция waveOutClose(), код ошибки - ");
		_itow((int)ReturnResult, s, 10);
		wcscat(PlayMsgBoxErrString, s);

		PlayResult.store(1); // флаг ошибки в потоке воспроизведения
	}

	hPlay_device = NULL;

	// освобождаем ресурсы playThread после остановки этого потока
	// перерисовываем кнопки
	// меняем область OpenGL (рисуем весь график звука) и перерисовываем главное окно 
	PostMessage(hWnd, WM_USER + 4, 0, 0);

	return;

}


// Callback-функция для waveIn (поток записи)
void CALLBACK waveRecProc(HWAVEIN hRecDev, UINT uMsg, DWORD_PTR dwInstance, \
	DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	switch (uMsg)
	{
	case MM_WIM_OPEN: // устройство записи успешно открыто
	{
		// пишем в hList1 сообщение об успешном открытии устройства записи
		PostMessage(hWnd, WM_USER + 10, 0, 0);

	} // MM_WIM_OPEN
	break;

	case MM_WIM_DATA:
	{
		if (waveHdr[AktivRecBufer].dwBytesRecorded != 0) // если записалось больше 0 байт в буфер записи
		{
			Last_rec_Mutex.lock(); // блокируем использование Rec_Last_rec в других потоках

			// выделяем память для нашего односвязного списка данных
			
			// если только визуализация (без сохранения)
			if (RecNoSave.load() == true)
			{
				// если ещё не выделили память под буфер визуализации
				if (Rec_Last_rec == NULL)
				{
					Rec_Last_rec = new rec_list;
					Rec_Last_rec->data = new char[BufferSize];
					Rec_Last_rec->next = NULL;
				}
			
			} // если только визуализация (без сохранения)

			// если запись и визуализация
			else
			{
				// если только запустили запись
				if(Head_rec == NULL)
				{
					// если память ещё ни разу не выделялась
					if (Rec_Last_rec == NULL)
					{
						Rec_Last_rec = new rec_list;
						Rec_Last_rec->data = new char[BufferSize];
						Rec_Last_rec->next = NULL;

						Head_rec = Rec_Last_rec;
					}
					// если память уже выделялась
					// (была включена предварительная визуализация)
					else
					{
						Head_rec = Rec_Last_rec;
						// выводим сообщение об успешном начале записи в hList1
						PostMessage(hWnd, WM_USER + 11, 0, 0);
					}
				
				} // если только запустили запись

				// запись уже идёт
				else
				{
					Rec_Last_rec->next = new rec_list;
					Rec_Last_rec = Rec_Last_rec->next;
					Rec_Last_rec->data = new char[BufferSize];
					Rec_Last_rec->next = NULL;
				}

			} // если запись и визуализация

			// записуем полученный буфер в наш односвязный список данных

			// если буфер записи полностью заполнен данными
			if (waveHdr[AktivRecBufer].dwBytesRecorded == BufferSize)
			{
				memcpy(Rec_Last_rec->data, waveHdr[AktivRecBufer].lpData, BufferSize);
			}
			// если буфер заполнен лишь частично, то остаток заполняем нулями (тишиной)
			else
			{
				memcpy(Rec_Last_rec->data, waveHdr[AktivRecBufer].lpData, \
					waveHdr[AktivRecBufer].dwBytesRecorded);

				if (struct_rec_data.wBitsPerSample == 8)
				{
					// 8-bit PCM silence = 0x80
					memset(Rec_Last_rec->data + waveHdr[AktivRecBufer].dwBytesRecorded, \
						0x80, (BufferSize - waveHdr[AktivRecBufer].dwBytesRecorded));
				}
				else {
					// 16/24/32-bit PCM silence = 0x00
					memset(Rec_Last_rec->data + waveHdr[AktivRecBufer].dwBytesRecorded, \
						0x00, (BufferSize - waveHdr[AktivRecBufer].dwBytesRecorded));
				}
			}

			Last_rec_Mutex.unlock(); // разблокируем использование Rec_Last_rec в других потоках


			// если запись и визуализация
			if (RecNoSave.load() == false)
			{
				// длина записи увеличилась
				RecBufferLenght_Mutex.lock(); // блокируем RecBufferLenght в других потоках

				::RecBufferLenght += ::BufferSize; // длина записи увеличилась
				::AudioTimeMS = ::RecBufferLenght / ::mySamplesPerSec / (::myBitsPerSample / 8) / ::myChannels * 1000;

				RecBufferLenght_Mutex.unlock(); // разблокируем RecBufferLenght в других потоках
			
			} // если запись и визуализация


			waveHdr[AktivRecBufer].dwBytesRecorded = 0; // показать что текущий буфер записи освобождён

			// пока флаг остановки == false - добавляем буфер в очередь записи
			if (RecStopFlag.load() == false)
			{
				MMRESULT ReturnResult = waveInAddBuffer(hRecDev, &waveHdr[AktivRecBufer], \
					sizeof(WAVEHDR));

				if (ReturnResult != MMSYSERR_NOERROR)
				{
					WCHAR s[15];

					wcscat(RecMsgBoxErrString, L"\nФункция waveInAddBuffer(х) завершилась с ошибкой \nКод ошибки - ");
					_itow(ReturnResult, s, 10);
					wcscat(RecMsgBoxErrString, s);

					RecResult.store(1); // поток записи завершился с ошибкой

					RecStopFlag.store(true); // флаг остановки, останавливаем запись
				}

			} // пока флаг остановки == false

			PostMessage(hWnd, WM_USER + 1, 0, 0); // перерисовываем главное окно

		} // если записалось больше 0 байт в буфер записи

		AktivRecBufer = !AktivRecBufer; // теперь активным будет другой буфер

	} // case MM_WIM_DATA
	break;

	default:
		break;
	}
}

// Функция потока записи: запускает запись и ждёт сигнала остановки
void RecordingThreadFunc()
{
	WCHAR s[250]; // буферные строки
	WCHAR s1[300]; // буферные строки

	// Пытаемся открыть устр. записи

	::AktivRecBufer = 0; // при новой записи указываем что активный опять первый буфер!

	RecResult.store(0); // по умолчанию у нас успех во время записи
	wcscpy(RecMsgBoxErrString, L"Возникли ошибки устройства записи: "); // первая строка, если будут ошибки

	UINT ReturnResult; // временно храним результат, возвращаемый функциями waveIn

	ReturnResult = waveInOpen(&hRec_device, waveInDevNummer,
		&struct_rec_data, (DWORD_PTR)waveRecProc, NULL, CALLBACK_FUNCTION); // отдельная функция

	//  --- если всё ОК, то тут в waveRecProc будет посылатся сообщение MM_WIM_OPEN ---

	// если не удалось открыть устройство записи
	if (ReturnResult != MMSYSERR_NOERROR)
	{
		// этот код никогда не должен сработать,
		// так как до этого в главном потоке
		// уже было тестовое открытие устройства

		RecStopFlag.store(true); // флаг остановки

		waveInClose(hRec_device); // на всякий случай
		hRec_device = NULL;

		// сообщение об ошибке в главное окно
		wcscat(RecMsgBoxErrString, L"\nФункция waveInOpen() завершилась с ошибкой \nКод ошибки - ");
		_itow(ReturnResult, s, 10);
		wcscat(RecMsgBoxErrString, s);

		RecResult.store(1); // поток записи завершился с ошибкой

		// помечаем поток как остановленный
		// меняем состояние кнопок
		// отображаем сообщение об ошибке
		PostMessage(hWnd, WM_USER + 12, 0, 0);

		return;

	} // не удалось открыть устройство записи

	// устройство записи успешно открыто
	// инициализируем 2 буфера записи

	//буфер 1
	if (waveHdr[0].lpData == NULL)
	{
		waveHdr[0].dwBufferLength = ::BufferSize; // размер буфера - 1/20 секунд или 1/19
		waveHdr[0].lpData = new char[::BufferSize];// память под буфер (байт)
	}
	waveHdr[0].dwFlags = NULL; // флаги потом система установит сама

	ReturnResult = waveInPrepareHeader(hRec_device, &waveHdr[0], sizeof(WAVEHDR)); // готовим заголовок буфера

	if (ReturnResult != MMSYSERR_NOERROR)
	{
		RecStopFlag.store(true); // флаг остановки

		waveInClose(hRec_device);
		hRec_device = NULL;

		wcscat(RecMsgBoxErrString, L"\nФункция waveInPrepareHeader(1) завершилась с ошибкой \nКод ошибки - ");
		_itow(ReturnResult, s, 10);
		wcscat(RecMsgBoxErrString, s);

		RecResult.store(1); // поток записи завершился с ошибкой

		// помечаем поток как остановленный
		// меняем состояние кнопок
		// отображаем сообщение об ошибке
		PostMessage(hWnd, WM_USER + 12, 0, 0);

		return;
	}

	// буфер 2
	if (waveHdr[1].lpData == NULL) {
		waveHdr[1].dwBufferLength = ::BufferSize; // размер буфера - 1/20 секунд или 1/19
		waveHdr[1].lpData = new char[::BufferSize];// память под буфер (байт)
	}
	waveHdr[1].dwFlags = NULL; // флаги потом система установит сама

	ReturnResult = waveInPrepareHeader(hRec_device, &waveHdr[1], sizeof(WAVEHDR)); // готовим заголовок буфера

	if (ReturnResult != MMSYSERR_NOERROR)
	{
		RecStopFlag.store(true); // флаг остановки

		// максимум 3 секунды пытаемся освободить уже подготовленный буфер
		DWORD start = GetTickCount();
		while ((waveHdr[0].dwFlags & WHDR_PREPARED) && (GetTickCount() - start < 3000))
		{
			waveInUnprepareHeader(hRec_device, &waveHdr[0], sizeof(WAVEHDR));
			Sleep(1);
		}

		waveInClose(hRec_device);
		hRec_device = NULL;

		wcscat(RecMsgBoxErrString, L"\nФункция waveInPrepareHeader(2) завершилась с ошибкой \nКод ошибки - ");
		_itow(ReturnResult, s, 10);
		wcscat(RecMsgBoxErrString, s);

		RecResult.store(1); // поток записи завершился с ошибкой

		// помечаем поток как остановленный
		// меняем состояние кнопок
		// отображаем сообщение об ошибке
		PostMessage(hWnd, WM_USER + 12, 0, 0);

		return;
	}

	// если буфер 1 и буфер 2 успешно подготовлены

	// Добавляем буфер_1 в очередь
	ReturnResult = waveInAddBuffer(hRec_device, &waveHdr[0], sizeof(WAVEHDR));

	if (ReturnResult != MMSYSERR_NOERROR)
	{
		RecStopFlag.store(true); // флаг остановки

		// максимум 3 секунды пытаемся освободить уже подготовленный буфер 1
		DWORD start = GetTickCount();
		while ((waveHdr[0].dwFlags & WHDR_PREPARED) && (GetTickCount() - start < 3000))
		{
			waveInUnprepareHeader(hRec_device, &waveHdr[0], sizeof(WAVEHDR));
			Sleep(1);
		}

		// максимум 3 секунды пытаемся освободить уже подготовленный буфер 2
		start = GetTickCount();
		while ((waveHdr[1].dwFlags & WHDR_PREPARED) && (GetTickCount() - start < 3000))
		{
			waveInUnprepareHeader(hRec_device, &waveHdr[1], sizeof(WAVEHDR));
			Sleep(1);
		}

		waveInClose(hRec_device);
		hRec_device = NULL;

		wcscat(RecMsgBoxErrString, L"\nФункция waveInAddBuffer(1) завершилась с ошибкой \nКод ошибки - ");
		_itow(ReturnResult, s, 10);
		wcscat(RecMsgBoxErrString, s);

		RecResult.store(1); // поток записи завершился с ошибкой

		// помечаем поток как остановленный
		// меняем состояние кнопок
		// отображаем сообщение об ошибке
		PostMessage(hWnd, WM_USER + 12, 0, 0);

		return;
	}

	// Добавляем буфер_2 в очередь
	ReturnResult = waveInAddBuffer(hRec_device, &waveHdr[1], sizeof(WAVEHDR));

	if (ReturnResult != MMSYSERR_NOERROR)
	{
		RecStopFlag.store(true); // флаг остановки

		waveInReset(hRec_device);

		// максимум 3 секунды пытаемся освободить уже подготовленный буфер 1
		DWORD start = GetTickCount();
		while ((waveHdr[0].dwFlags & WHDR_PREPARED) && (GetTickCount() - start < 3000))
		{
			waveInUnprepareHeader(hRec_device, &waveHdr[0], sizeof(WAVEHDR));
			Sleep(1);
		}

		// максимум 3 секунды пытаемся освободить уже подготовленный буфер 2
		start = GetTickCount();
		while ((waveHdr[1].dwFlags & WHDR_PREPARED) && (GetTickCount() - start < 3000))
		{
			waveInUnprepareHeader(hRec_device, &waveHdr[1], sizeof(WAVEHDR));
			Sleep(1);
		}

		waveInClose(hRec_device);
		hRec_device = NULL;

		wcscat(RecMsgBoxErrString, L"\nФункция waveInAddBuffer(2) завершилась с ошибкой \nКод ошибки - ");
		_itow(ReturnResult, s, 10);
		wcscat(RecMsgBoxErrString, s);

		RecResult.store(1); // поток записи завершился с ошибкой

		// помечаем поток как остановленный
		// меняем состояние кнопок
		// отображаем сообщение об ошибке
		PostMessage(hWnd, WM_USER + 12, 0, 0);

		return;
	}

	// пытаемся начать запись
	ReturnResult = waveInStart(hRec_device);

	if (ReturnResult != MMSYSERR_NOERROR)
	{
		RecStopFlag.store(true); // флаг остановки

		waveInReset(hRec_device);

		// максимум 3 секунды пытаемся освободить уже подготовленный буфер 1
		DWORD start = GetTickCount();
		while ((waveHdr[0].dwFlags & WHDR_PREPARED) && (GetTickCount() - start < 3000))
		{
			waveInUnprepareHeader(hRec_device, &waveHdr[0], sizeof(WAVEHDR));
			Sleep(1);
		}

		// максимум 3 секунды пытаемся освободить уже подготовленный буфер 2
		start = GetTickCount();
		while ((waveHdr[1].dwFlags & WHDR_PREPARED) && (GetTickCount() - start < 3000))
		{
			waveInUnprepareHeader(hRec_device, &waveHdr[1], sizeof(WAVEHDR));
			Sleep(1);
		}

		waveInClose(hRec_device);
		hRec_device = NULL;

		wcscat(RecMsgBoxErrString, L"\nФункция waveInStart() завершилась с ошибкой \nКод ошибки - ");
		_itow(ReturnResult, s, 10);
		wcscat(RecMsgBoxErrString, s);

		RecResult.store(1); // поток записи завершился с ошибкой

		// помечаем поток как остановленный
		// меняем состояние кнопок
		// отображаем сообщение об ошибке
		PostMessage(hWnd, WM_USER + 12, 0, 0);

		return;
	}

	// если запись успешно запущена
	else
	{
		// выводим сообщение об успехе в hList1
		PostMessage(hWnd, WM_USER + 11, 0, 0);
	}

	// если не было ошибок при запуске записи
	if (RecResult.load() == 0)
	{
		// поток записи работает, пока флаг RecStopFlag равен false (проверяем раз в 10 мс)
		while (RecStopFlag.load() != true)
		{
			Sleep(10);
		}
	}


	// Получили команду остановки записи от главного потока через флаг RecStopFlag
	// Останавливаем и очищаем всё, что связано с waveIn

	// пытаемся остановить запись
	ReturnResult = waveInStop(hRec_device); // проверка возвращаемого значения

	if (ReturnResult != MMSYSERR_NOERROR)
	{
		wcscat(RecMsgBoxErrString, L"\nВ вызове waveInStop() ошибка: ");
		_itow(ReturnResult, s, 10);
		wcscat(RecMsgBoxErrString, s);

		RecResult.store(1);
	}

	waveInReset(hRec_device);

	// максимум 3 секунды пытаемся освободить уже подготовленный буфер 1
	DWORD start = GetTickCount();
	while ((waveHdr[0].dwFlags & WHDR_PREPARED) && (GetTickCount() - start < 3000))
	{
		ReturnResult = waveInUnprepareHeader(hRec_device, &waveHdr[0], sizeof(WAVEHDR));
		Sleep(1);
	}

	if (ReturnResult != MMSYSERR_NOERROR)
	{
		wcscat(RecMsgBoxErrString, L"\nВ вызове waveInUnprepareHeader(1) ошибка: ");
		_itow(ReturnResult, s, 10);
		wcscat(RecMsgBoxErrString, s);

		RecResult.store(1);
	}

	// максимум 3 секунды пытаемся освободить уже подготовленный буфер 2
	start = GetTickCount();
	while ((waveHdr[1].dwFlags & WHDR_PREPARED) && (GetTickCount() - start < 3000))
	{
		ReturnResult = waveInUnprepareHeader(hRec_device, &waveHdr[1], sizeof(WAVEHDR));
		Sleep(1);
	}

	if (ReturnResult != MMSYSERR_NOERROR)
	{
		wcscat(RecMsgBoxErrString, L"\nВ вызове waveInUnprepareHeader(2) ошибка: ");
		_itow(ReturnResult, s, 10);
		wcscat(RecMsgBoxErrString, s);

		RecResult.store(1);
	}

	ReturnResult = waveInClose(hRec_device);
	if (ReturnResult != MMSYSERR_NOERROR)
	{
		wcscat(RecMsgBoxErrString, L"\nВ вызове waveInClose() ошибка: ");
		_itow(ReturnResult, s, 10);
		wcscat(RecMsgBoxErrString, s);

		RecResult.store(1);
	}

	// всегда, даже если waveInClose() с ошибками
	hRec_device = NULL;
 
	// освобождаем ресурсы recThread после остановки этого потока
	// перерисовываем кнопки
	// меняем область OpenGL (рисуем весь график звука) и перерисовываем главное окно
	// выводим сообщение об ошибке если она есть
	PostMessage(hWnd, WM_USER + 14, 0, 0);

	return;
}


void PrepareSoundGraph(rec_list* Head, ULONG BufferSize, double RecBufferLenght)
{

	mySoundGraph.Ready = false; // запрещаем отображать график, пока не вычислим его

	// начальные проверки
	if (Head == NULL)
	{
		return;
	}
	if (BufferSize < 1)
	{
		return;
	}
	// если длина звука слишком короткая, то невозможно вычислить граф звука
	// 16 = 4 байта (максимум 32 бит звук)  * 2 канала (максимум) * 2 раза (мин и макс замеры)
	if (RecBufferLenght < mySoundGraph.Length * 16)
	{
		return;
	}


	/////////////////////////////////////////////////////////////////////////////
	// делаем мини окошко "Подготовка графика звука..."
	HWND hWndWaitWindow = NULL;
	// Если длина аудио более 10 мин. (для моно, 44КГц, 16 бит)
	if (RecBufferLenght > 52920000.0)
	{
		// определяем текущий масштаб в системе
		float Scale = (float)WindowDPI / 96.0f; // масштаб окна (в Win XP равен 1.0)

		int widthWaitWindow = int(240 * Scale); // ширина нашего мини окна
		int heightWaitWindow = int(80 * Scale); // высота нашего мини окна

		// получаем размеры и координаты главного окна
		RECT rcParent;
		GetWindowRect(hWnd, &rcParent);

		// мини окно всегда будет в центре главного окна
		int xWaitWindow = rcParent.left + (rcParent.right - rcParent.left - widthWaitWindow) / 2;
		int yWaitWindow = rcParent.top + (rcParent.bottom - rcParent.top - heightWaitWindow) / 2;

		// создаём мини окно
		hWndWaitWindow = CreateWindowEx(
			WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
			L"STATIC",
			L"Подготовка графика звука...",
			WS_POPUP | WS_BORDER | WS_VISIBLE | SS_CENTER,
			xWaitWindow, yWaitWindow, widthWaitWindow, heightWaitWindow,
			hWnd, NULL, hInst, NULL);

		// меняем стиль шрифта мини окна
		SendMessage(hWndWaitWindow, WM_SETFONT, (WPARAM)hFontButton, TRUE);

		UpdateWindow(hWndWaitWindow);

		// ... и блокируем ввод в главное окно (пока идёт обработка данных
		EnableWindow(hWnd, FALSE);

	}
	/////////////////////////////////////////////////////////////////////////////


	// создаём массив ссылок на все буферы нашего односвязного списка
	rec_list** AudioDataArray;

	ULONG BufferLength = (ULONG)round(RecBufferLenght / (double)BufferSize);

	AudioDataArray = new rec_list * [BufferLength];

	ULONG i, j, k;

	rec_list* LastBuf = Head;
	for (i = 0; i < BufferLength; i++)
	{
		AudioDataArray[i] = LastBuf;
		LastBuf = LastBuf->next;
	}

	double startIndex = 0; // индексы данных внутри (0,RecBufferLenght)
	double endIndex; // индексы данных внутри (0,RecBufferLenght)

	ULONG StartBufNumber;
	ULONG EndBufNumber;

	ULONG StartBufIndex;
	ULONG EndBufIndex;

	// вычисляем график mono звука
	if (myChannels == 1)
	{
		double AudioBufferData; // буферная переменная

		// глубина звука 8 бит
		if (myBitsPerSample == 8)
		{

			// для всех точек будущего графика звука (кроме последней)
			for (i = 0; i < mySoundGraph.Length - 1; i++)
			{
				StartBufNumber = (ULONG)std::floor(startIndex / (double)BufferSize);
				StartBufIndex = (ULONG)std::fmod(startIndex, (double)BufferSize);

				endIndex = std::floor(RecBufferLenght / (double)mySoundGraph.Length * (i + 1));

				EndBufNumber = (ULONG)std::floor(endIndex / (double)BufferSize);
				EndBufIndex = (ULONG)std::fmod(endIndex, (double)BufferSize);

				mySoundGraph.DataL[i][0] = 128.0; // наш минимум, начальные присваивания
				mySoundGraph.DataL[i][1] = -128.0; // наш максимум, начальные присваивания

				// ищем локальный минимум и максимум в вычисленном фрагменте данных
				for (j = StartBufNumber; j <= EndBufNumber; j++)
				{
					if (j == StartBufNumber)
					{
						if (j == EndBufNumber)
						{
							for (k = StartBufIndex; k < EndBufIndex; k++)
							{
								AudioBufferData = (double)(unsigned char)AudioDataArray[j]->data[k];
								AudioBufferData -= 128.0;

								// если данные меньше текущего минимума
								if (AudioBufferData < mySoundGraph.DataL[i][0])
									mySoundGraph.DataL[i][0] = AudioBufferData;
								// если данные больше текущего максимума
								if (AudioBufferData > mySoundGraph.DataL[i][1])
									mySoundGraph.DataL[i][1] = AudioBufferData;
							}
						} //if (j == EndBufNumber)

						else // (j != EndBufNumber)
						{
							for (k = StartBufIndex; k < BufferSize; k++)
							{
								AudioBufferData = (double)(unsigned char)AudioDataArray[j]->data[k];
								AudioBufferData -= 128.0;

								// если данные меньше текущего минимума
								if (AudioBufferData < mySoundGraph.DataL[i][0])
									mySoundGraph.DataL[i][0] = AudioBufferData;
								// если данные больше текущего максимума
								if (AudioBufferData > mySoundGraph.DataL[i][1])
									mySoundGraph.DataL[i][1] = AudioBufferData;
							}
						} // (j != EndBufNumber)

					} // if(j == StartBufNumber)

					else // if (j != StartBufNumber)
					{
						if (j == EndBufNumber)
						{
							for (k = 0; k < EndBufIndex; k++)
							{
								AudioBufferData = (double)(unsigned char)AudioDataArray[j]->data[k];
								AudioBufferData -= 128.0;

								// если данные меньше текущего минимума
								if (AudioBufferData < mySoundGraph.DataL[i][0])
									mySoundGraph.DataL[i][0] = AudioBufferData;
								// если данные больше текущего максимума
								if (AudioBufferData > mySoundGraph.DataL[i][1])
									mySoundGraph.DataL[i][1] = AudioBufferData;
							}
						} //if (j == EndBufNumber)

						else //if (j != EndBufNumber)
						{
							for (k = 0; k < BufferSize; k++)
							{
								AudioBufferData = (double)(unsigned char)AudioDataArray[j]->data[k];
								AudioBufferData -= 128.0;

								// если данные меньше текущего минимума
								if (AudioBufferData < mySoundGraph.DataL[i][0])
									mySoundGraph.DataL[i][0] = AudioBufferData;
								// если данные больше текущего максимума
								if (AudioBufferData > mySoundGraph.DataL[i][1])
									mySoundGraph.DataL[i][1] = AudioBufferData;
							}
						} // if (j != EndBufNumber)

					} // if (j != StartBufNumber)

				} // ищем локальный минимум и максимум в вычисленном фрагменте данных

				// так как endIndex мы не задействовали (k < EndBufIndex)
				// в следующий раз начинаем с него
				startIndex = endIndex;

			} // для всех точек будущего графика звука (кроме последней)

			// для последней точки будущего графика звука
			// !!! временный код, надо переделать !!!!!
			{
				mySoundGraph.DataL[mySoundGraph.Length - 1][0] = 0.0;
				mySoundGraph.DataL[mySoundGraph.Length - 1][1] = 0.0;
			}

			mySoundGraph.Ready = true; // вычислили график звука, можно выводить

		} // if (myBitsPerSample == 8)

		// глубина звука 16 бит
		if (myBitsPerSample == 16)
		{

			short AudioBufferDataShort; // буферная переменная

			// для всех точек будущего графика звука (кроме последней)
			for (i = 0; i < mySoundGraph.Length - 1; i++)
			{
				StartBufNumber = (ULONG)std::floor(startIndex / (double)BufferSize);
				StartBufIndex = (ULONG)std::fmod(startIndex, (double)BufferSize);

				endIndex = std::floor(RecBufferLenght / (double)mySoundGraph.Length * (i + 1));
				// выравнивание - последний байт должен быть нечётным
				if (std::fmod(endIndex, 2) != 1.0)
				{
					endIndex -= 1.0;
				}

				EndBufNumber = (ULONG)std::floor(endIndex / (double)BufferSize);
				EndBufIndex = (ULONG)std::fmod(endIndex, (double)BufferSize);

				mySoundGraph.DataL[i][0] = 32768.0; // наш минимум, начальные присваивания
				mySoundGraph.DataL[i][1] = -32768.0; // наш максимум, начальные присваивания

				// ищем локальный минимум и максимум в вычисленном фрагменте данных
				for (j = StartBufNumber; j <= EndBufNumber; j++)
				{
					if (j == StartBufNumber)
					{
						if (j == EndBufNumber)
						{
							for (k = StartBufIndex; k < EndBufIndex; k += 2)
							{
								// код только для Little Endian (WAV) 
								AudioBufferDataShort = \
									static_cast<unsigned char>(AudioDataArray[j]->data[k]) |
									(static_cast<unsigned char>(AudioDataArray[j]->data[k + 1]) << 8);
								//
								AudioBufferData = (double)AudioBufferDataShort;

								// если данные меньше текущего минимума
								if (AudioBufferData < mySoundGraph.DataL[i][0])
									mySoundGraph.DataL[i][0] = AudioBufferData;
								// если данные больше текущего максимума
								if (AudioBufferData > mySoundGraph.DataL[i][1])
									mySoundGraph.DataL[i][1] = AudioBufferData;
							}
						} //if (j == EndBufNumber)

						else // (j != EndBufNumber)
						{
							for (k = StartBufIndex; k < BufferSize; k += 2)
							{
								// код только для Little Endian (WAV) 
								AudioBufferDataShort = \
									static_cast<unsigned char>(AudioDataArray[j]->data[k]) |
									(static_cast<unsigned char>(AudioDataArray[j]->data[k + 1]) << 8);
								//
								AudioBufferData = (double)AudioBufferDataShort;

								// если данные меньше текущего минимума
								if (AudioBufferData < mySoundGraph.DataL[i][0])
									mySoundGraph.DataL[i][0] = AudioBufferData;
								// если данные больше текущего максимума
								if (AudioBufferData > mySoundGraph.DataL[i][1])
									mySoundGraph.DataL[i][1] = AudioBufferData;
							}
						} // (j != EndBufNumber)

					} // if(j == StartBufNumber)

					else // if(j != StartBufNumber)
					{
						if (j == EndBufNumber)
						{
							for (k = 0; k < EndBufIndex; k += 2)
							{
								// код только для Little Endian (WAV) 
								AudioBufferDataShort = \
									static_cast<unsigned char>(AudioDataArray[j]->data[k]) |
									(static_cast<unsigned char>(AudioDataArray[j]->data[k + 1]) << 8);
								//
								AudioBufferData = (double)AudioBufferDataShort;

								// если данные меньше текущего минимума
								if (AudioBufferData < mySoundGraph.DataL[i][0])
									mySoundGraph.DataL[i][0] = AudioBufferData;
								// если данные больше текущего максимума
								if (AudioBufferData > mySoundGraph.DataL[i][1])
									mySoundGraph.DataL[i][1] = AudioBufferData;
							}

						} //if (j == EndBufNumber)

						else // if (j != EndBufNumber)
						{
							for (k = 0; k < BufferSize; k += 2)
							{
								// код только для Little Endian (WAV) 
								AudioBufferDataShort = \
									static_cast<unsigned char>(AudioDataArray[j]->data[k]) |
									(static_cast<unsigned char>(AudioDataArray[j]->data[k + 1]) << 8);
								//
								AudioBufferData = (double)AudioBufferDataShort;

								// если данные меньше текущего минимума
								if (AudioBufferData < mySoundGraph.DataL[i][0])
									mySoundGraph.DataL[i][0] = AudioBufferData;
								// если данные больше текущего максимума
								if (AudioBufferData > mySoundGraph.DataL[i][1])
									mySoundGraph.DataL[i][1] = AudioBufferData;
							}
						} // if (j != EndBufNumber)

					} // if (j != StartBufNumber)

				} // ищем локальный минимум и максимум в вычисленном фрагменте данных

				startIndex = endIndex + 1; // тут всё правильно (отличие от 8 бит звука)

			} // для всех точек будущего графика звука (кроме последней)

			// для последней точки будущего графика звука
			// !!! временный код, надо переделать !!!!!
			{
				mySoundGraph.DataL[mySoundGraph.Length - 1][0] = 0.0;
				mySoundGraph.DataL[mySoundGraph.Length - 1][1] = 0.0;
			}

			mySoundGraph.Ready = true; // вычислили график звука, можно выводить

		} // if (myBitsPerSample == 16)

		// глубина звука 24 бит
		if (myBitsPerSample == 24)
		{

			// для всех точек будущего графика звука (кроме последней)
			for (i = 0; i < mySoundGraph.Length - 1; i++)
			{
				StartBufNumber = (ULONG)std::floor(startIndex / (double)BufferSize);
				StartBufIndex = (ULONG)std::fmod(startIndex, (double)BufferSize);

				endIndex = std::floor(RecBufferLenght / (double)mySoundGraph.Length * (i + 1));
				// выравнивание - последний байт должен быть равным 2 
				// (0,1,-2-, 3, 4, -5-,...) при делении на цело на 3
				if (std::fmod(endIndex, 3) != 2.0)
				{
					endIndex -= 1.0; // отнимаем
					if (std::fmod(endIndex, 3) != 2.0)
					{
						endIndex -= 1.0; // максимум 2 раза отнимаем
					}
				}

				EndBufNumber = (ULONG)std::floor(endIndex / (double)BufferSize);
				EndBufIndex = (ULONG)std::fmod(endIndex, (double)BufferSize);

				mySoundGraph.DataL[i][0] = 8388608.0; // наш минимум, начальные присваивания
				mySoundGraph.DataL[i][1] = -8388608.0; // наш максимум, начальные присваивания

				// ищем локальный минимум и максимум в вычисленном фрагменте данных
				for (j = StartBufNumber; j <= EndBufNumber; j++)
				{
					if (j == StartBufNumber)
					{
						if (j == EndBufNumber)
						{
							for (k = StartBufIndex; k < EndBufIndex; k += 3)
							{
								AudioBufferData =
									convertSampleToDouble((uint8_t*)&AudioDataArray[j]->data[k]);

								// если данные меньше текущего минимума
								if (AudioBufferData < mySoundGraph.DataL[i][0])
									mySoundGraph.DataL[i][0] = AudioBufferData;
								// если данные больше текущего максимума
								if (AudioBufferData > mySoundGraph.DataL[i][1])
									mySoundGraph.DataL[i][1] = AudioBufferData;
							}
						} //if (j == EndBufNumber)

						else // (j != EndBufNumber)
						{
							for (k = StartBufIndex; k < BufferSize; k += 3)
							{
								AudioBufferData =
									convertSampleToDouble((uint8_t*)&AudioDataArray[j]->data[k]);

								// если данные меньше текущего минимума
								if (AudioBufferData < mySoundGraph.DataL[i][0])
									mySoundGraph.DataL[i][0] = AudioBufferData;
								// если данные больше текущего максимума
								if (AudioBufferData > mySoundGraph.DataL[i][1])
									mySoundGraph.DataL[i][1] = AudioBufferData;
							}
						} // (j != EndBufNumber)

					} // if(j == StartBufNumber)

					else // if(j != StartBufNumber)
					{
						if (j == EndBufNumber)
						{
							for (k = 0; k < EndBufIndex; k += 3)
							{
								AudioBufferData =
									convertSampleToDouble((uint8_t*)&AudioDataArray[j]->data[k]);

								// если данные меньше текущего минимума
								if (AudioBufferData < mySoundGraph.DataL[i][0])
									mySoundGraph.DataL[i][0] = AudioBufferData;
								// если данные больше текущего максимума
								if (AudioBufferData > mySoundGraph.DataL[i][1])
									mySoundGraph.DataL[i][1] = AudioBufferData;
							}
						} //if (j == EndBufNumber)

						else //if (j != EndBufNumber)
						{
							for (k = 0; k < BufferSize; k += 3)
							{
								AudioBufferData =
									convertSampleToDouble((uint8_t*)&AudioDataArray[j]->data[k]);

								// если данные меньше текущего минимума
								if (AudioBufferData < mySoundGraph.DataL[i][0])
									mySoundGraph.DataL[i][0] = AudioBufferData;
								// если данные больше текущего максимума
								if (AudioBufferData > mySoundGraph.DataL[i][1])
									mySoundGraph.DataL[i][1] = AudioBufferData;
							}
						} //if (j != EndBufNumber)

					} // if(j != StartBufNumber)

				} // ищем локальный минимум и максимум в вычисленном фрагменте данных

				startIndex = endIndex + 1; // тут всё правильно (отличие от 8 бит звука)

			} // для всех точек будущего графика звука (кроме последней)

			// для последней точки будущего графика звука
			// !!! временный код, надо переделать !!!!!
			{
				mySoundGraph.DataL[mySoundGraph.Length - 1][0] = 0.0;
				mySoundGraph.DataL[mySoundGraph.Length - 1][1] = 0.0;
			}

			mySoundGraph.Ready = true; // вычислили график звука, можно выводить

		} // if (myBitsPerSample == 24)

	} // if (myChannels == 1)

	// вычисляем график stereo звука
	if (myChannels == 2)
	{
		double AudioBufferData; // буферная переменная

		// глубина звука 8 бит
		if (myBitsPerSample == 8)
		{

			// для всех точек будущего графика звука (кроме последней)
			for (i = 0; i < mySoundGraph.Length - 1; i++)
			{
				StartBufNumber = (ULONG)std::floor(startIndex / (double)BufferSize);
				StartBufIndex = (ULONG)std::fmod(startIndex, (double)BufferSize);

				endIndex = std::floor(RecBufferLenght / (double)mySoundGraph.Length * (i + 1));
				// выравнивание - последний байт должен быть нечётным
				if (std::fmod(endIndex, 2) != 1.0)
				{
					endIndex -= 1.0;
				}

				EndBufNumber = (ULONG)std::floor(endIndex / (double)BufferSize);
				EndBufIndex = (ULONG)std::fmod(endIndex, (double)BufferSize);

				mySoundGraph.DataL[i][0] = 128.0; // наш минимум, начальные присваивания
				mySoundGraph.DataL[i][1] = -128.0; // наш максимум, начальные присваивания

				mySoundGraph.DataR[i][0] = 128.0; // наш минимум, начальные присваивания
				mySoundGraph.DataR[i][1] = -128.0; // наш максимум, начальные присваивания

				// ищем локальный минимум и максимум в вычисленном фрагменте данных
				for (j = StartBufNumber; j <= EndBufNumber; j++)
				{
					if (j == StartBufNumber)
					{
						if (j == EndBufNumber)
						{
							for (k = StartBufIndex; k < EndBufIndex;)
							{
								// левый канал
								AudioBufferData = (double)(unsigned char)AudioDataArray[j]->data[k];
								AudioBufferData -= 128.0;
								// если данные меньше текущего минимума
								if (AudioBufferData < mySoundGraph.DataL[i][0])
									mySoundGraph.DataL[i][0] = AudioBufferData;
								// если данные больше текущего максимума
								if (AudioBufferData > mySoundGraph.DataL[i][1])
									mySoundGraph.DataL[i][1] = AudioBufferData;

								k += 1;

								// правый канал
								AudioBufferData = (double)(unsigned char)AudioDataArray[j]->data[k];
								AudioBufferData -= 128.0;
								// если данные меньше текущего минимума
								if (AudioBufferData < mySoundGraph.DataR[i][0])
									mySoundGraph.DataR[i][0] = AudioBufferData;
								// если данные больше текущего максимума
								if (AudioBufferData > mySoundGraph.DataR[i][1])
									mySoundGraph.DataR[i][1] = AudioBufferData;

								k += 1;

							}
						} //if (j == EndBufNumber)

						else // (j != EndBufNumber)
						{
							for (k = StartBufIndex; k < BufferSize;)
							{
								// левый канал
								AudioBufferData = (double)(unsigned char)AudioDataArray[j]->data[k];
								AudioBufferData -= 128.0;
								// если данные меньше текущего минимума
								if (AudioBufferData < mySoundGraph.DataL[i][0])
									mySoundGraph.DataL[i][0] = AudioBufferData;
								// если данные больше текущего максимума
								if (AudioBufferData > mySoundGraph.DataL[i][1])
									mySoundGraph.DataL[i][1] = AudioBufferData;

								k += 1;

								// правый канал
								AudioBufferData = (double)(unsigned char)AudioDataArray[j]->data[k];
								AudioBufferData -= 128.0;
								// если данные меньше текущего минимума
								if (AudioBufferData < mySoundGraph.DataR[i][0])
									mySoundGraph.DataR[i][0] = AudioBufferData;
								// если данные больше текущего максимума
								if (AudioBufferData > mySoundGraph.DataR[i][1])
									mySoundGraph.DataR[i][1] = AudioBufferData;

								k += 1;

							}
						} // (j != EndBufNumber)

					} // if(j == StartBufNumber)

					else // if (j != StartBufNumber)
					{
						if (j == EndBufNumber)
						{
							for (k = 0; k < EndBufIndex;)
							{
								// левый канал
								AudioBufferData = (double)(unsigned char)AudioDataArray[j]->data[k];
								AudioBufferData -= 128.0;
								// если данные меньше текущего минимума
								if (AudioBufferData < mySoundGraph.DataL[i][0])
									mySoundGraph.DataL[i][0] = AudioBufferData;
								// если данные больше текущего максимума
								if (AudioBufferData > mySoundGraph.DataL[i][1])
									mySoundGraph.DataL[i][1] = AudioBufferData;

								k += 1;

								// правый канал
								AudioBufferData = (double)(unsigned char)AudioDataArray[j]->data[k];
								AudioBufferData -= 128.0;
								// если данные меньше текущего минимума
								if (AudioBufferData < mySoundGraph.DataR[i][0])
									mySoundGraph.DataR[i][0] = AudioBufferData;
								// если данные больше текущего максимума
								if (AudioBufferData > mySoundGraph.DataR[i][1])
									mySoundGraph.DataR[i][1] = AudioBufferData;

								k += 1;

							}
						} //if (j == EndBufNumber)

						else //if (j != EndBufNumber)
						{
							for (k = 0; k < BufferSize;)
							{
								// левый канал
								AudioBufferData = (double)(unsigned char)AudioDataArray[j]->data[k];
								AudioBufferData -= 128.0;
								// если данные меньше текущего минимума
								if (AudioBufferData < mySoundGraph.DataL[i][0])
									mySoundGraph.DataL[i][0] = AudioBufferData;
								// если данные больше текущего максимума
								if (AudioBufferData > mySoundGraph.DataL[i][1])
									mySoundGraph.DataL[i][1] = AudioBufferData;

								k += 1;

								// правый канал
								AudioBufferData = (double)(unsigned char)AudioDataArray[j]->data[k];
								AudioBufferData -= 128.0;
								// если данные меньше текущего минимума
								if (AudioBufferData < mySoundGraph.DataR[i][0])
									mySoundGraph.DataR[i][0] = AudioBufferData;
								// если данные больше текущего максимума
								if (AudioBufferData > mySoundGraph.DataR[i][1])
									mySoundGraph.DataR[i][1] = AudioBufferData;

								k += 1;

							}
						} // if (j != EndBufNumber)

					} // if (j != StartBufNumber)

				} // ищем локальный минимум и максимум в вычисленном фрагменте данных

				startIndex = endIndex + 1;

			} // для всех точек будущего графика звука (кроме последней)

			// для последней точки будущего графика звука
			// !!! временный код, надо переделать !!!!!
			{
				// левый канал
				mySoundGraph.DataL[mySoundGraph.Length - 1][0] = 0.0;
				mySoundGraph.DataL[mySoundGraph.Length - 1][1] = 0.0;

				// правый канал
				mySoundGraph.DataR[mySoundGraph.Length - 1][0] = 0.0;
				mySoundGraph.DataR[mySoundGraph.Length - 1][1] = 0.0;
			}

			mySoundGraph.Ready = true; // вычислили график звука, можно выводить

		} // if (myBitsPerSample == 8)

		// глубина звука 16 бит
		if (myBitsPerSample == 16)
		{

			short AudioBufferDataShort; // буферная переменная для 16 бит

			// для всех точек будущего графика звука (кроме последней)
			for (i = 0; i < mySoundGraph.Length - 1; i++)
			{
				StartBufNumber = (ULONG)std::floor(startIndex / (double)BufferSize);
				StartBufIndex = (ULONG)std::fmod(startIndex, (double)BufferSize);

				endIndex = std::floor(RecBufferLenght / (double)mySoundGraph.Length * (i + 1));
				// выравнивание - последний байт
				// при делениие на цело на 4 должен давать 3 (3, 7, 11,...)
				while (std::fmod(endIndex, 4) != 3.0)
				{
					endIndex -= 1.0;
				}

				EndBufNumber = (ULONG)std::floor(endIndex / (double)BufferSize);
				EndBufIndex = (ULONG)std::fmod(endIndex, (double)BufferSize);

				// левый канал
				mySoundGraph.DataL[i][0] = 32768.0; // наш минимум, начальные присваивания
				mySoundGraph.DataL[i][1] = -32768.0; // наш максимум, начальные присваивания

				// правый канал
				mySoundGraph.DataR[i][0] = 32768.0; // наш минимум, начальные присваивания
				mySoundGraph.DataR[i][1] = -32768.0; // наш максимум, начальные присваивания


				// ищем локальный минимум и максимум в вычисленном фрагменте данных
				for (j = StartBufNumber; j <= EndBufNumber; j++)
				{
					if (j == StartBufNumber)
					{
						if (j == EndBufNumber)
						{
							for (k = StartBufIndex; k < EndBufIndex;)
							{
								// левый канал
								// код только для Little Endian (WAV) 
								AudioBufferDataShort = \
									static_cast<unsigned char>(AudioDataArray[j]->data[k]) |
									(static_cast<unsigned char>(AudioDataArray[j]->data[k + 1]) << 8);
								//
								AudioBufferData = (double)AudioBufferDataShort;

								// если данные меньше текущего минимума
								if (AudioBufferData < mySoundGraph.DataL[i][0])
									mySoundGraph.DataL[i][0] = AudioBufferData;
								// если данные больше текущего максимума
								if (AudioBufferData > mySoundGraph.DataL[i][1])
									mySoundGraph.DataL[i][1] = AudioBufferData;

								k += 2; // 16 бит или 2 байта вперёд


								// правый канал
								// код только для Little Endian (WAV) 
								AudioBufferDataShort = \
									static_cast<unsigned char>(AudioDataArray[j]->data[k]) |
									(static_cast<unsigned char>(AudioDataArray[j]->data[k + 1]) << 8);
								//
								AudioBufferData = (double)AudioBufferDataShort;

								// если данные меньше текущего минимума
								if (AudioBufferData < mySoundGraph.DataR[i][0])
									mySoundGraph.DataR[i][0] = AudioBufferData;
								// если данные больше текущего максимума
								if (AudioBufferData > mySoundGraph.DataR[i][1])
									mySoundGraph.DataR[i][1] = AudioBufferData;

								k += 2; // 16 бит или 2 байта вперёд
							}
						} //if (j == EndBufNumber)

						else // (j != EndBufNumber)
						{
							for (k = StartBufIndex; k < BufferSize;)
							{
								// левый канал
								// код только для Little Endian (WAV) 
								AudioBufferDataShort = \
									static_cast<unsigned char>(AudioDataArray[j]->data[k]) |
									(static_cast<unsigned char>(AudioDataArray[j]->data[k + 1]) << 8);
								//
								AudioBufferData = (double)AudioBufferDataShort;

								// если данные меньше текущего минимума
								if (AudioBufferData < mySoundGraph.DataL[i][0])
									mySoundGraph.DataL[i][0] = AudioBufferData;
								// если данные больше текущего максимума
								if (AudioBufferData > mySoundGraph.DataL[i][1])
									mySoundGraph.DataL[i][1] = AudioBufferData;

								k += 2; // 16 бит или 2 байта вперёд


								// правый канал
								// код только для Little Endian (WAV) 
								AudioBufferDataShort = \
									static_cast<unsigned char>(AudioDataArray[j]->data[k]) |
									(static_cast<unsigned char>(AudioDataArray[j]->data[k + 1]) << 8);
								//
								AudioBufferData = (double)AudioBufferDataShort;

								// если данные меньше текущего минимума
								if (AudioBufferData < mySoundGraph.DataR[i][0])
									mySoundGraph.DataR[i][0] = AudioBufferData;
								// если данные больше текущего максимума
								if (AudioBufferData > mySoundGraph.DataR[i][1])
									mySoundGraph.DataR[i][1] = AudioBufferData;

								k += 2; // 16 бит или 2 байта вперёд
							}
						} // (j != EndBufNumber)

					} // if(j == StartBufNumber)

					else // if(j != StartBufNumber)
					{
						if (j == EndBufNumber)
						{
							for (k = 0; k < EndBufIndex;)
							{
								// левый канал
								// код только для Little Endian (WAV) 
								AudioBufferDataShort = \
									static_cast<unsigned char>(AudioDataArray[j]->data[k]) |
									(static_cast<unsigned char>(AudioDataArray[j]->data[k + 1]) << 8);
								//
								AudioBufferData = (double)AudioBufferDataShort;

								// если данные меньше текущего минимума
								if (AudioBufferData < mySoundGraph.DataL[i][0])
									mySoundGraph.DataL[i][0] = AudioBufferData;
								// если данные больше текущего максимума
								if (AudioBufferData > mySoundGraph.DataL[i][1])
									mySoundGraph.DataL[i][1] = AudioBufferData;

								k += 2; // 16 бит или 2 байта вперёд


								// правый канал
								// код только для Little Endian (WAV) 
								AudioBufferDataShort = \
									static_cast<unsigned char>(AudioDataArray[j]->data[k]) |
									(static_cast<unsigned char>(AudioDataArray[j]->data[k + 1]) << 8);
								//
								AudioBufferData = (double)AudioBufferDataShort;

								// если данные меньше текущего минимума
								if (AudioBufferData < mySoundGraph.DataR[i][0])
									mySoundGraph.DataR[i][0] = AudioBufferData;
								// если данные больше текущего максимума
								if (AudioBufferData > mySoundGraph.DataR[i][1])
									mySoundGraph.DataR[i][1] = AudioBufferData;

								k += 2; // 16 бит или 2 байта вперёд
							}

						} //if (j == EndBufNumber)

						else // if (j != EndBufNumber)
						{
							for (k = 0; k < BufferSize;)
							{
								// левый канал
								// код только для Little Endian (WAV) 
								AudioBufferDataShort = \
									static_cast<unsigned char>(AudioDataArray[j]->data[k]) |
									(static_cast<unsigned char>(AudioDataArray[j]->data[k + 1]) << 8);
								//
								AudioBufferData = (double)AudioBufferDataShort;

								// если данные меньше текущего минимума
								if (AudioBufferData < mySoundGraph.DataL[i][0])
									mySoundGraph.DataL[i][0] = AudioBufferData;
								// если данные больше текущего максимума
								if (AudioBufferData > mySoundGraph.DataL[i][1])
									mySoundGraph.DataL[i][1] = AudioBufferData;

								k += 2; // 16 бит или 2 байта вперёд


								// правый канал
								// код только для Little Endian (WAV) 
								AudioBufferDataShort = \
									static_cast<unsigned char>(AudioDataArray[j]->data[k]) |
									(static_cast<unsigned char>(AudioDataArray[j]->data[k + 1]) << 8);
								//
								AudioBufferData = (double)AudioBufferDataShort;

								// если данные меньше текущего минимума
								if (AudioBufferData < mySoundGraph.DataR[i][0])
									mySoundGraph.DataR[i][0] = AudioBufferData;
								// если данные больше текущего максимума
								if (AudioBufferData > mySoundGraph.DataR[i][1])
									mySoundGraph.DataR[i][1] = AudioBufferData;

								k += 2; // 16 бит или 2 байта вперёд
							}
						} // if (j != EndBufNumber)

					} // if (j != StartBufNumber)

				} // ищем локальный минимум и максимум в вычисленном фрагменте данных

				startIndex = endIndex + 1;

			} // для всех точек будущего графика звука (кроме последней)

			// для последней точки будущего графика звука
			// !!! временный код, надо переделать !!!!!
			{
				// левый канал
				mySoundGraph.DataL[mySoundGraph.Length - 1][0] = 0.0;
				mySoundGraph.DataL[mySoundGraph.Length - 1][1] = 0.0;

				// правый канал
				mySoundGraph.DataR[mySoundGraph.Length - 1][0] = 0.0;
				mySoundGraph.DataR[mySoundGraph.Length - 1][1] = 0.0;
			}

			mySoundGraph.Ready = true; // вычислили график звука, можно выводить

		} // if (myBitsPerSample == 16)

		// глубина звука 24 бит
		if (myBitsPerSample == 24)
		{

			// для всех точек будущего графика звука (кроме последней)
			for (i = 0; i < mySoundGraph.Length - 1; i++)
			{
				StartBufNumber = (ULONG)std::floor(startIndex / (double)BufferSize);
				StartBufIndex = (ULONG)std::fmod(startIndex, (double)BufferSize);

				endIndex = std::floor(RecBufferLenght / (double)mySoundGraph.Length * (i + 1));
				// выравнивание - последний байт должен быть равным 5 
				// (0,1,2, 3, 4, -5-,...) при делении на цело на 6
				while (std::fmod(endIndex, 6) != 5.0)
				{
					endIndex -= 1.0; // уменьшаем
				}

				EndBufNumber = (ULONG)std::floor(endIndex / (double)BufferSize);
				EndBufIndex = (ULONG)std::fmod(endIndex, (double)BufferSize);

				// левый канал
				mySoundGraph.DataL[i][0] = 8388608.0; // наш минимум, начальные присваивания
				mySoundGraph.DataL[i][1] = -8388608.0; // наш максимум, начальные присваивания

				// правый канал
				mySoundGraph.DataR[i][0] = 8388608.0; // наш минимум, начальные присваивания
				mySoundGraph.DataR[i][1] = -8388608.0; // наш максимум, начальные присваивания

				// ищем локальный минимум и максимум в вычисленном фрагменте данных
				for (j = StartBufNumber; j <= EndBufNumber; j++)
				{
					if (j == StartBufNumber)
					{
						if (j == EndBufNumber)
						{
							for (k = StartBufIndex; k < EndBufIndex;)
							{
								// левый канал
								AudioBufferData =
									convertSampleToDouble((uint8_t*)&AudioDataArray[j]->data[k]);

								// если данные меньше текущего минимума
								if (AudioBufferData < mySoundGraph.DataL[i][0])
									mySoundGraph.DataL[i][0] = AudioBufferData;
								// если данные больше текущего максимума
								if (AudioBufferData > mySoundGraph.DataL[i][1])
									mySoundGraph.DataL[i][1] = AudioBufferData;

								k += 3;

								// правый канал
								AudioBufferData =
									convertSampleToDouble((uint8_t*)&AudioDataArray[j]->data[k]);

								// если данные меньше текущего минимума
								if (AudioBufferData < mySoundGraph.DataR[i][0])
									mySoundGraph.DataR[i][0] = AudioBufferData;
								// если данные больше текущего максимума
								if (AudioBufferData > mySoundGraph.DataR[i][1])
									mySoundGraph.DataR[i][1] = AudioBufferData;

								k += 3;

							}
						} //if (j == EndBufNumber)

						else // (j != EndBufNumber)
						{
							for (k = StartBufIndex; k < BufferSize;)
							{
								// левый канал
								AudioBufferData =
									convertSampleToDouble((uint8_t*)&AudioDataArray[j]->data[k]);

								// если данные меньше текущего минимума
								if (AudioBufferData < mySoundGraph.DataL[i][0])
									mySoundGraph.DataL[i][0] = AudioBufferData;
								// если данные больше текущего максимума
								if (AudioBufferData > mySoundGraph.DataL[i][1])
									mySoundGraph.DataL[i][1] = AudioBufferData;

								k += 3;

								// правый канал
								AudioBufferData =
									convertSampleToDouble((uint8_t*)&AudioDataArray[j]->data[k]);

								// если данные меньше текущего минимума
								if (AudioBufferData < mySoundGraph.DataR[i][0])
									mySoundGraph.DataR[i][0] = AudioBufferData;
								// если данные больше текущего максимума
								if (AudioBufferData > mySoundGraph.DataR[i][1])
									mySoundGraph.DataR[i][1] = AudioBufferData;

								k += 3;

							}
						} // (j != EndBufNumber)

					} // if(j == StartBufNumber)

					else // if(j != StartBufNumber)
					{
						if (j == EndBufNumber)
						{
							for (k = 0; k < EndBufIndex;)
							{
								// левый канал
								AudioBufferData =
									convertSampleToDouble((uint8_t*)&AudioDataArray[j]->data[k]);

								// если данные меньше текущего минимума
								if (AudioBufferData < mySoundGraph.DataL[i][0])
									mySoundGraph.DataL[i][0] = AudioBufferData;
								// если данные больше текущего максимума
								if (AudioBufferData > mySoundGraph.DataL[i][1])
									mySoundGraph.DataL[i][1] = AudioBufferData;

								k += 3;

								// правый канал
								AudioBufferData =
									convertSampleToDouble((uint8_t*)&AudioDataArray[j]->data[k]);

								// если данные меньше текущего минимума
								if (AudioBufferData < mySoundGraph.DataR[i][0])
									mySoundGraph.DataR[i][0] = AudioBufferData;
								// если данные больше текущего максимума
								if (AudioBufferData > mySoundGraph.DataR[i][1])
									mySoundGraph.DataR[i][1] = AudioBufferData;

								k += 3;

							}
						} //if (j == EndBufNumber)

						else //if (j != EndBufNumber)
						{
							for (k = 0; k < BufferSize;)
							{
								// левый канал
								AudioBufferData =
									convertSampleToDouble((uint8_t*)&AudioDataArray[j]->data[k]);

								// если данные меньше текущего минимума
								if (AudioBufferData < mySoundGraph.DataL[i][0])
									mySoundGraph.DataL[i][0] = AudioBufferData;
								// если данные больше текущего максимума
								if (AudioBufferData > mySoundGraph.DataL[i][1])
									mySoundGraph.DataL[i][1] = AudioBufferData;

								k += 3;

								// правый канал
								AudioBufferData =
									convertSampleToDouble((uint8_t*)&AudioDataArray[j]->data[k]);

								// если данные меньше текущего минимума
								if (AudioBufferData < mySoundGraph.DataR[i][0])
									mySoundGraph.DataR[i][0] = AudioBufferData;
								// если данные больше текущего максимума
								if (AudioBufferData > mySoundGraph.DataR[i][1])
									mySoundGraph.DataR[i][1] = AudioBufferData;

								k += 3;

							}
						} //if (j != EndBufNumber)

					} // if(j != StartBufNumber)

				} // ищем локальный минимум и максимум в вычисленном фрагменте данных

				startIndex = endIndex + 1;

			} // для всех точек будущего графика звука (кроме последней)

			// для последней точки будущего графика звука
			// !!! временный код, надо переделать !!!!!
			{
				// левый канал
				mySoundGraph.DataL[mySoundGraph.Length - 1][0] = 0.0;
				mySoundGraph.DataL[mySoundGraph.Length - 1][1] = 0.0;

				// правый канал
				mySoundGraph.DataR[mySoundGraph.Length - 1][0] = 0.0;
				mySoundGraph.DataR[mySoundGraph.Length - 1][1] = 0.0;

			}

			mySoundGraph.Ready = true; // вычислили график звука, можно выводить

		} // if (myBitsPerSample == 24)

	} // if (myChannels == 2)


	delete[] AudioDataArray; // освобождаем память указателей на данные (сами данные остались)

	/////////////////////////////////////////////////////////////////////////////
	// если выводили, то убираем мини окошко "Подготовка графика звука..."
	if (hWndWaitWindow != NULL)
	{
		DestroyWindow(hWndWaitWindow);
		// ...снова активируем главное окно
		EnableWindow(hWnd, TRUE);
		// и возвращаем фокус главному окну
		SetForegroundWindow(hWnd);
	}
	/////////////////////////////////////////////////////////////////////////////

}

void ShowSoundGraph(BOOL localShowPeaks)
{
	if (mySoundGraph.Ready == false)
		return;

	ULONG i;
	double xPos, xPosNext;

	// отображаем график mono звука
	if (myChannels == 1)
	{
		glColor3d(0.0, 0.5, 0.0); // цвет графика моно звука

		// для всех точек mySoundGraph рисуем  линии мин-макс
		glBegin(GL_LINES);

		// для всех точек mySoundGraph (кроме последней)
		for (i = 0; i < mySoundGraph.Length - 1; i++)
		{
			xPos = glXmax / mySoundGraph.Length * i;

			glVertex2d(xPos, mySoundGraph.DataL[i][0]);
			glVertex2d(xPos, mySoundGraph.DataL[i][1]);

			// если есть разрыв между соседними сегментами
			xPosNext = glXmax / mySoundGraph.Length * (i + 1);

			// если максимум текущего сегмента меньше минимума следующего (линия вверх)
			if (mySoundGraph.DataL[i][1] < mySoundGraph.DataL[i + 1][0])
			{
				glVertex2d(xPos, mySoundGraph.DataL[i][1]);
				glVertex2d(xPosNext, mySoundGraph.DataL[i + 1][0]);
			}
			// если минимум текущего сегмента больше максимума следующего (линия вниз)
			else if (mySoundGraph.DataL[i][0] > mySoundGraph.DataL[i + 1][1])
			{
				glVertex2d(xPos, mySoundGraph.DataL[i][0]);
				glVertex2d(xPosNext, mySoundGraph.DataL[i + 1][1]);
			}

		} // для всех точек mySoundGraph (кроме последней)

		// для последней точки mySoundGraph
		xPos = glXmax / mySoundGraph.Length * i;

		glVertex2d(xPos, mySoundGraph.DataL[i][0]);
		glVertex2d(xPos, mySoundGraph.DataL[i][1]);

		/* // тестовый код, рисуем вертикальную красную линию там, где был клиппинг
		glColor3d(1.0, 0.2, 0.2); // возвращаем цвет линии графика

		for (i = 0; i < mySoundGraph.Length - 1; i++)
		{
			xPos = glXmax / mySoundGraph.Length * i;

			if ((mySoundGraph.DataL[i][0]/glYmin) > 0.99 || (mySoundGraph.DataL[i][1]/glYmax) > 0.99)
			{
				glVertex2d(xPos, glYmin); // рисуем фон заданным цветом ..
				glVertex2d(xPos, glYmax); // в виде линии от минимуму до максимума (снизу вверх)
			}
		}
		// конец тестовый код */

		glEnd();

		// если длина линии мин-макс слишком короткая, рисуем мин и макс точками
		glBegin(GL_POINTS);

		for (i = 0; i < mySoundGraph.Length; i++)
		{
			xPos = glXmax / mySoundGraph.Length * i;

			glVertex2d(xPos, mySoundGraph.DataL[i][0]);
			glVertex2d(xPos, mySoundGraph.DataL[i][1]);

		}

		glEnd();

	} // if (myChannels == 1)

	// отображаем график stereo звука
	if (myChannels == 2)
	{
		double YOffset = 0; // смещение графика для левого(вверх) и для правого канала(вниз) 

		if (myBitsPerSample == 8)
			YOffset = 64; // диапазон от -128 до +127, смещаем на пол диапазона (+-64)
		if (myBitsPerSample == 16)
			YOffset = 16383.5;
		if (myBitsPerSample == 24)
			YOffset = 0.5;


		//// левый канал
		glColor3d(0.0, 0.5, 0.0); // цвет графика левого канала

		// для всех точек левого канала mySoundGraph рисуем  линии мин-макс
		glBegin(GL_LINES);

		// для всех точек mySoundGraph (кроме последней)
		for (i = 0; i < mySoundGraph.Length - 1; i++)
		{
			xPos = glXmax / mySoundGraph.Length * i;

			glVertex2d(xPos, mySoundGraph.DataL[i][0] / 2 + YOffset);
			glVertex2d(xPos, mySoundGraph.DataL[i][1] / 2 + YOffset);

			// если есть разрыв между соседними сегментами
			xPosNext = glXmax / mySoundGraph.Length * (i + 1);

			// если максимум текущего сегмента меньше минимума следующего (линия вверх)
			if (mySoundGraph.DataL[i][1] < mySoundGraph.DataL[i + 1][0])
			{
				glVertex2d(xPos, mySoundGraph.DataL[i][1] / 2 + YOffset);
				glVertex2d(xPosNext, mySoundGraph.DataL[i + 1][0] / 2 + YOffset);
			}
			// если минимум текущего сегмента больше максимума следующего (линия вниз)
			else if (mySoundGraph.DataL[i][0] > mySoundGraph.DataL[i + 1][1])
			{
				glVertex2d(xPos, mySoundGraph.DataL[i][0] / 2 + YOffset);
				glVertex2d(xPosNext, mySoundGraph.DataL[i + 1][1] / 2 + YOffset);
			}

		} // для всех точек mySoundGraph (кроме последней)

		// для последней точки mySoundGraph
		xPos = glXmax / mySoundGraph.Length * i;

		glVertex2d(xPos, mySoundGraph.DataL[i][0] / 2 + YOffset);
		glVertex2d(xPos, mySoundGraph.DataL[i][1] / 2 + YOffset);

		glEnd();

		// если длина линии мин-макс слишком короткая, рисуем мин и макс точками
		glBegin(GL_POINTS);

		for (i = 0; i < mySoundGraph.Length; i++)
		{
			xPos = glXmax / mySoundGraph.Length * i;

			glVertex2d(xPos, mySoundGraph.DataL[i][0] / 2 + YOffset);
			glVertex2d(xPos, mySoundGraph.DataL[i][1] / 2 + YOffset);

		}

		glEnd();


		//// правый канал
		glColor3d(0.0, 0.0, 0.6);// цвет графика правого канала

		// для всех точек правого канала mySoundGraph рисуем  линии мин-макс
		glBegin(GL_LINES);

		// для всех точек mySoundGraph (кроме последней)
		for (i = 0; i < mySoundGraph.Length - 1; i++)
		{
			xPos = glXmax / mySoundGraph.Length * i;

			glVertex2d(xPos, mySoundGraph.DataR[i][0] / 2 - YOffset);
			glVertex2d(xPos, mySoundGraph.DataR[i][1] / 2 - YOffset);

			// если есть разрыв между соседними сегментами
			xPosNext = glXmax / mySoundGraph.Length * (i + 1);

			// если максимум текущего сегмента меньше минимума следующего (линия вверх)
			if (mySoundGraph.DataR[i][1] < mySoundGraph.DataR[i + 1][0])
			{
				glVertex2d(xPos, mySoundGraph.DataR[i][1] / 2 - YOffset);
				glVertex2d(xPosNext, mySoundGraph.DataR[i + 1][0] / 2 - YOffset);
			}
			// если минимум текущего сегмента больше максимума следующего (линия вниз)
			else if (mySoundGraph.DataR[i][0] > mySoundGraph.DataR[i + 1][1])
			{
				glVertex2d(xPos, mySoundGraph.DataR[i][0] / 2 - YOffset);
				glVertex2d(xPosNext, mySoundGraph.DataR[i + 1][1] / 2 - YOffset);
			}

		} // для всех точек mySoundGraph (кроме последней)

		// для последней точки mySoundGraph
		xPos = glXmax / mySoundGraph.Length * i;

		glVertex2d(xPos, mySoundGraph.DataR[i][0] / 2 - YOffset);
		glVertex2d(xPos, mySoundGraph.DataR[i][1] / 2 - YOffset);

		glEnd();

		// если длина линии мин-макс слишком короткая, рисуем мин и макс точками
		glBegin(GL_POINTS);

		for (i = 0; i < mySoundGraph.Length; i++)
		{
			xPos = glXmax / mySoundGraph.Length * i;

			glVertex2d(xPos, mySoundGraph.DataR[i][0] / 2 - YOffset);
			glVertex2d(xPos, mySoundGraph.DataR[i][1] / 2 - YOffset);

		}

		glEnd();

	} // if (myChannels == 2)

	// если нужно, отображаем пики/перегрузку
	if (localShowPeaks == TRUE)
	{
		// мин и макс значения для 8, 16, 24 битного звука
		double minY, maxY;

		// порог срабатывания перегрузки для 8, 16, 24 битного звука (99.99% от мин/макс)
		double minYThreshold, maxYThreshold;

		double xPos; // буферная переменная

		if (myBitsPerSample == 8)
		{
			minY = -128.0;
			maxY = 127.0;

			minYThreshold = -127.99;
			maxYThreshold = 126.99;

		}

		if (myBitsPerSample == 16)
		{
			minY = -32768.0;
			maxY = 32767.0;

			minYThreshold = -32764.0;
			maxYThreshold = 32763.0;

		}

		if (myBitsPerSample == 24)
		{
			minY = -1.0;
			maxY = 1.0;

			minYThreshold = -0.9999;
			maxYThreshold = 0.9999;
		}

		glColor3d(1.0, 0.2, 0.2); // цвет для линий пиков/перегрузки (красный)

		// mono звук
		if (myChannels == 1)
		{
			glBegin(GL_LINES);

			// для всех точек mySoundGraph
			for (ULONG i = 0; i < mySoundGraph.Length; i++)
			{
				// если значения текущего мин или максимума (mySoundGraph.DataL[i])
				// вышли за допустимые переделы (minYThreshold или maxYThreshold)
				// значит фиксируем и визуализируем пик/перегрузку
				if (mySoundGraph.DataL[i][0] < minYThreshold || mySoundGraph.DataL[i][1] > maxYThreshold)
				{
					xPos = glXmax / mySoundGraph.Length * i;

					glVertex2d(xPos, minY);
					glVertex2d(xPos, maxY);
				}

			} // для всех точек mySoundGraph

			glEnd();

		} // if (myChannels == 1)


		// stereo звук
		if (myChannels == 2)
		{
			glBegin(GL_LINES);

			// левый канал

			// для всех точек mySoundGraph
			for (ULONG i = 0; i < mySoundGraph.Length; i++)
			{
				// если значения текущего мин или максимума (mySoundGraph.DataL[i])
				// вышли за допустимые переделы (minYThreshold или maxYThreshold)
				// значит фиксируем и визуализируем пик/перегрузку
				if (mySoundGraph.DataL[i][0] < minYThreshold || mySoundGraph.DataL[i][1] > maxYThreshold)
				{
					xPos = glXmax / mySoundGraph.Length * i;

					// координаты вертикальной линии перегрузки для левого канала (в верхней части)
					glVertex2d(xPos, 0.0);
					glVertex2d(xPos, maxY);
				}

			} // для всех точек mySoundGraph


			// правый канал

			// для всех точек mySoundGraph
			for (ULONG i = 0; i < mySoundGraph.Length; i++)
			{
				// если значения текущего мин или максимума (mySoundGraph.DataR[i])
				// вышли за допустимые переделы (minYThreshold или maxYThreshold)
				// значит фиксируем и визуализируем пик/перегрузку
				if (mySoundGraph.DataR[i][0] < minYThreshold || mySoundGraph.DataR[i][1] > maxYThreshold)
				{
					xPos = glXmax / mySoundGraph.Length * i;

					// координаты вертикальной линии перегрузки для правого канала (в нижней части)
					glVertex2d(xPos, minY);
					glVertex2d(xPos, 0.0);
				}

			} // для всех точек mySoundGraph

			glEnd();

		} // if (myChannels == 2)

	} // if (localShowPeaks == TRUE)


}


INT_PTR CALLBACK VolumeDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	
	// инициализация диалогового окна
	case WM_INITDIALOG:
	{
		// окно в центре внутри родительского окна
		HWND hOwner = GetWindow(hDlg, GW_OWNER);
		if (!hOwner)
			hOwner = GetDesktopWindow(); // fallback: центрирование относительно экрана

		RECT rcOwner, rcDlg;

		GetWindowRect(hOwner, &rcOwner);
		// если родительское окно свёрнуто, размеры == 0, 
		if ((rcOwner.right - rcOwner.left) == 0 || (rcOwner.bottom - rcOwner.top) == 0)
			SystemParametersInfo(SPI_GETWORKAREA, 0, &rcOwner, 0); // центрируем по рабочему столу

		GetWindowRect(hDlg, &rcDlg);

		int dlgWidth = rcDlg.right - rcDlg.left;
		int dlgHeight = rcDlg.bottom - rcDlg.top;

		int posX = rcOwner.left + ((rcOwner.right - rcOwner.left) - dlgWidth) / 2;
		int posY = rcOwner.top + ((rcOwner.bottom - rcOwner.top) - dlgHeight) / 2;

		MoveWindow(hDlg, posX, posY, dlgWidth, dlgHeight, TRUE);


		// Устанавливаем диапазон для слайдера (от 10% до 400% от исходной громкости) 
		SendDlgItemMessage(hDlg, IDC_VOLUMECHANGE_SLIDER, TBM_SETRANGE, TRUE, MAKELPARAM(10, 400));
		
		// Устанавливаем шаг для слайдера (шаг 10 единиц ) 
		SendDlgItemMessage(hDlg, IDC_VOLUMECHANGE_SLIDER, TBM_SETPAGESIZE, 0, 10);

		// Устанавливаем частоту разметки, каждые 10 единиц
		SendDlgItemMessage(hDlg, IDC_VOLUMECHANGE_SLIDER, TBM_SETTICFREQ, 10, 0);

		// Устанавливаем начальное значение - 100%
		SendDlgItemMessage(hDlg, IDC_VOLUMECHANGE_SLIDER, TBM_SETPOS, TRUE, 100);

		// Устанавливаем начальное значение для текста 
		SetDlgItemText(hDlg, IDC_VOLUMECHANGE_STATIC, L"Новый уровень громкости: 100%");

		// Устанавливаем кнопку "Отмена" основной 
		SendMessage(hDlg, DM_SETDEFID, IDC_VOLUMECHANGE_CANCEL, 0);

		return (INT_PTR)TRUE;

	} // WM_INITDIALOG

	// Обработка перемещения ползунка слайдера 
	case WM_HSCROLL: 
	{
		if ((HWND)lParam == GetDlgItem(hDlg, IDC_VOLUMECHANGE_SLIDER))
		{
			int pos = SendDlgItemMessage(hDlg, IDC_VOLUMECHANGE_SLIDER, TBM_GETPOS, 0, 0);
			WCHAR buffer[50];
			swprintf(buffer, 50, L"Новый уровень громкости: %i%%", pos);
			SetDlgItemText(hDlg, IDC_VOLUMECHANGE_STATIC, buffer);
		}

		break;

	} // WM_HSCROLL
	
	// обработка кнопок
	case WM_COMMAND:

		if (LOWORD(wParam) == IDC_VOLUMECHANGE_APPLY) {

			// Получаем текущее значение слайдера 
			int volumePercent = SendDlgItemMessage(hDlg, IDC_VOLUMECHANGE_SLIDER, TBM_GETPOS, 0, 0);

			// Преобразуем значение процентов в коэффициент умножения для звука
			double volumeMultiplier;
			volumeMultiplier = (double)volumePercent / 100.0;

			// делаем неактивными кнопки внутри диалогового окна
			EnableWindow(GetDlgItem(hDlg, IDC_VOLUMECHANGE_APPLY), FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_VOLUMECHANGE_CANCEL), FALSE);

			// Выводим информацию о выполнении обработки в диалоговое окно
			SetDlgItemText(hDlg, IDC_VOLUMECHANGE_STATIC, L"Изменение громкости...");
			
			// Обновляем громкость 
			if (ChVolume(Head_rec, BufferSize, volumeMultiplier) == true) // если успех
			{
				// Выводим информацию о выполнении обработки в диалоговое окно
				//SetDlgItemText(hDlg, IDC_VOLUMECHANGE_STATIC, L"Подготовка графика звука...");

				// обновить массив графа звука
				PrepareSoundGraph(Head_rec, BufferSize, RecBufferLenght);

				InvalidateRect(hWnd, NULL, FALSE);  // перерисовываем область OpenGL
			}

			// Sleep(5000);

			// Завершаем диалог и возвращаем значение 
			EndDialog(hDlg, LOWORD(wParam)); 
			return (INT_PTR)TRUE;

		} //IDC_VOLUMECHANGE_APPLY

		else if (LOWORD(wParam) == IDC_VOLUMECHANGE_CANCEL)
		{ 
			// Завершаем диалог и возвращаем значение 
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;

		} // IDC_VOLUMECHANGE_CANCEL
	
		break;  // WM_COMMAND

	case WM_CLOSE: 

		EndDialog(hDlg, LOWORD(wParam)); 
		return (INT_PTR)TRUE;

	} 
	
	return (INT_PTR)FALSE; // по умолчанию (если не обработали сообщение)
}

BOOL ShowAudioTime(HWND hWndOutput, double AudioDurationMS, double AudioCurrentPosMS)
{
	if (hWndOutput == NULL)
		return FALSE;

	WCHAR s2[25]; // буфер для текста
	WCHAR s1[10]; // буфер для текста

	int TimeHour, TimeMin, TimeSec; // буферные переменные, храним соответствующие величины для отобр.

	wcscpy(s2, L" "); // вначале всегда пробел

	// если аудио воспроизводится, дабавляем в начало строки текущее время воспр.
	if (AudioCurrentPosMS > 0)
	{
		TimeHour = AudioCurrentPosMS / 1000.0 / 3600.0;
		TimeMin = (AudioCurrentPosMS / 60000.0) - ((double)TimeHour * 60.0);
		TimeSec = (AudioCurrentPosMS / 1000.0) - double(TimeMin) * 60.0 - double(TimeHour) * 3600.0;

		// если длина аудио более часа
		if (TimeHour > 0)
		{
			// отображение часового индикатора
			_itow(TimeHour, s1, 10);
			wcscat(s2, s1);
			wcscat(s2, L":");

			// один из вариантов отображения минут
			if (TimeMin == 0)
				wcscat(s2, L"00:");
			else
			{
				if (TimeMin < 10)
				{
					wcscat(s2, L"0");
					_itow(TimeMin, s1, 10);
					wcscat(s2, s1);
					wcscat(s2, L":");
				}
				else
				{
					_itow(TimeMin, s1, 10);
					wcscat(s2, s1);
					wcscat(s2, L":");
				}

			} // один из вариантов отображения минут

			// один из вариантов отображения секунд
			if (TimeSec == 0)
				wcscat(s2, L"00");
			else
			{
				if (TimeSec < 10)
				{
					wcscat(s2, L"0");
					_itow(TimeSec, s1, 10);
					wcscat(s2, s1);
				}
				else
				{
					_itow(TimeSec, s1, 10);
					wcscat(s2, s1);
				}

			} // один из вариантов отображения секунд

		} // если длина аудио более часа

		// если длина аудио менее часа
		else 
		{

			// отображаем минуты
			if (TimeMin == 0)
				wcscat(s2, L"0:");
			else
			{
				_itow(TimeMin, s1, 10);
				wcscat(s2, s1);
				wcscat(s2, L":");

			}

			// один из вариантов отображения секунд
			if (TimeSec == 0)
				wcscat(s2, L"00");
			else
			{
				if (TimeSec < 10)
				{
					wcscat(s2, L"0");
					_itow(TimeSec, s1, 10);
					wcscat(s2, s1);
				}
				else
				{
					_itow(TimeSec, s1, 10);
					wcscat(s2, s1);
				}
			} // один из вариантов отображения секунд

		} // если длина аудио менее часа

		// в конец добавляем разделитель
		wcscat(s2, L" / ");

	}

	// добавляем в конец строки общую длину аудио
	TimeHour = AudioDurationMS / 1000 / 3600;
	TimeMin = (AudioDurationMS / 60000) - (TimeHour * 60);
	TimeSec = (AudioDurationMS / 1000) - double(TimeMin) * 60 - (double)TimeHour * 3600;

	// если длина аудио более часа
	if (TimeHour > 0)
	{
		// отображение часового индикатора
		_itow(TimeHour, s1, 10);
		wcscat(s2, s1);
		wcscat(s2, L":");

		// один из вариантов отображения минут
		if (TimeMin == 0)
			wcscat(s2, L"00:");
		else
		{
			if (TimeMin < 10)
			{
				wcscat(s2, L"0");
				_itow(TimeMin, s1, 10);
				wcscat(s2, s1);
				wcscat(s2, L":");
			}
			else
			{
				_itow(TimeMin, s1, 10);
				wcscat(s2, s1);
				wcscat(s2, L":");
			}

		} // один из вариантов отображения минут

		// один из вариантов отображения секунд
		if (TimeSec == 0)
			wcscat(s2, L"00");
		else
		{
			if (TimeSec < 10)
			{
				wcscat(s2, L"0");
				_itow(TimeSec, s1, 10);
				wcscat(s2, s1);
			}
			else
			{
				_itow(TimeSec, s1, 10);
				wcscat(s2, s1);
			}

		} // один из вариантов отображения секунд

	} // если длина аудио более часа

	// если длина аудио менее часа
	else
	{

		// отображаем минуты
		if (TimeMin == 0)
			wcscat(s2, L"0:");
		else
		{
			_itow(TimeMin, s1, 10);
			wcscat(s2, s1);
			wcscat(s2, L":");

		}

		// один из вариантов отображения секунд
		if (TimeSec == 0)
			wcscat(s2, L"00");
		else
		{
			if (TimeSec < 10)
			{
				wcscat(s2, L"0");
				_itow(TimeSec, s1, 10);
				wcscat(s2, s1);
			}
			else
			{
				_itow(TimeSec, s1, 10);
				wcscat(s2, s1);
			}
		} // один из вариантов отображения секунд

	} // если длина аудио менее часа

	static WCHAR OutputString[25] = {L""}; // инициализация только при первом вызове функции

	// если строка времени изменилась после прошлого вызова (посекундно),
	// то обновляем и перерисовываем текст в главном окне
	if (wcscmp(OutputString, s2) != 0)
	{
		wcscpy(OutputString, s2);
		SetWindowText(hWndOutput, s2);
	}

	return TRUE;

}

rec_list *GetAudioPosition(rec_list *Head, ULONG BufferSize, double TrackSize, double Pos, double* ReturnPos)
{
	if (Head == NULL) return NULL;
	if (TrackSize <= 0) return NULL;
	if (BufferSize <= 0) return NULL;
	if (Pos > 1 || Pos < 0) return NULL; // информация о позиции внутри аудио должна быть от 0 до 1

	double dCurrentPos = 0;
	rec_list* strCurrentPos = Head;

	// Преобразуем позицию из диапазона 0..1 в фактическое значение в байтах внутри файла
	Pos = TrackSize * Pos;
	Pos = Pos - (double)BufferSize; // всегда будем останавливаться на буфере, который чуть левее нашей позиции

	while (dCurrentPos < Pos)
	{
		// не должны дойти до края данных раньше чем нужно 
		if (strCurrentPos != NULL)
		{
			strCurrentPos = strCurrentPos->next;
			dCurrentPos += BufferSize;
		}
		// случайно дошли до края файла раньше чем надо
		else
			return NULL;
	}

	if(ReturnPos != NULL)
		*ReturnPos = dCurrentPos; //возвращаем информацию, сколько байт уже проиграно

	return strCurrentPos; // ссылка на текущий проигрываемый блок

}

void RedrawWindowElements(int W, int H, float Scale)
{
	// Scale = 1.0; // для тестов

	// изменение порта просмотра OpenGL
	{
		// для OpenGL отсчёт координат с левого нижнего угла
		glViewportX = 5;
		glViewportY = 256 * Scale;
		glViewportWidth = W - 10;
		glViewportHeight = H - 257 * Scale;

		if (glViewportWidth <= 0) glViewportWidth = 1;
		if (glViewportHeight <= 0) glViewportHeight = 1;

		// перерисовываем порт просмотра OpenGL
		reshape(glViewportX, glViewportY, glViewportWidth, glViewportHeight);
	}

	static float OldScale = 0.0f; // храним прошлый масштаб

	// если поменялся масштаб, 
	// то меняем размер шрифтов элементов окна
	if (fabsf(Scale - OldScale) > 0.001f)
	{
		OldScale = Scale;

		// шрифт для кнопок (hFontButton)
		int baseSize = 16; // логический размер
		int sizePx = (int)(baseSize * Scale + 0.5f); // фактический размер в пикселях

		HFONT hOld = NULL, hNew = NULL; // временные переменные для проверки успешного создания шрифта

		hNew = CreateFont(
			-sizePx,			// отрицательное значение = высота в пикселях 
			0,                 // Ширина (0 = автоматически)
			0,                 // Угол наклона строки (0 = без наклона)
			0,                 // Угол наклона шрифта
			FW_MEDIUM,         // Толщина (FW_NORMAL, FW_BOLD и т. д.)
			FALSE,             // Курсив
			FALSE,             // Подчёркивание
			FALSE,             // Зачёркивание
			DEFAULT_CHARSET,   // Кодовая страница (ANSI, DEFAULT, EASTEUROPE и т. д.)
			OUT_DEFAULT_PRECIS, // Точность вывода
			CLIP_DEFAULT_PRECIS, // Точность отсечения
			DEFAULT_QUALITY,   // Качество
			FF_SWISS,          // Семейство шрифта (FF_SWISS = "без засечек")
			L"Arial" // Имя шрифта 
		);
		if (hNew)
		{
			HFONT hOld = hFontButton;
			hFontButton = hNew;
			if (hOld) DeleteObject(hOld);
		}

		// шрифт для GroupBox и ComboBox (hFontComboBox)
		baseSize = 12; // логический размер
		sizePx = (int)(baseSize * Scale + 0.5f); // фактический размер в пикселях

		hNew = CreateFont(
			-sizePx,			// отрицательное значение = высота в пикселях 
			0,                 // Ширина (0 = автоматически)
			0,                 // Угол наклона строки (0 = без наклона)
			0,                 // Угол наклона шрифта
			FW_MEDIUM,         // Толщина (FW_NORMAL, FW_BOLD и т. д.)
			FALSE,             // Курсив
			FALSE,             // Подчёркивание
			FALSE,             // Зачёркивание
			DEFAULT_CHARSET,   // Кодовая страница (ANSI, DEFAULT, EASTEUROPE и т. д.)
			OUT_DEFAULT_PRECIS, // Точность вывода
			CLIP_DEFAULT_PRECIS, // Точность отсечения
			DEFAULT_QUALITY,   // Качество
			FF_SWISS,          // Семейство шрифта (FF_SWISS = "без засечек")
			L"Arial" // Имя шрифта
		);
		if (hNew)
		{
			HFONT hOld = hFontComboBox;
			hFontComboBox = hNew;
			if (hOld) DeleteObject(hOld);
		}

		// шрифт для hList1, мельче и тоньше (hFonthList1)
		baseSize = 10; // логический размер
		sizePx = (int)(baseSize * Scale + 0.5f); // фактический размер в пикселях

		hNew = CreateFont(
			-sizePx,			// отрицательное значение = высота в пикселях
			0,                 // Ширина (0 = автоматически)
			0,                 // Угол наклона строки (0 = без наклона)
			0,                 // Угол наклона шрифта
			FW_NORMAL,         // Толщина (FW_NORMAL, FW_BOLD и т. д.)
			FALSE,             // Курсив
			FALSE,             // Подчёркивание
			FALSE,             // Зачёркивание
			DEFAULT_CHARSET,   // Кодовая страница (ANSI, DEFAULT, EASTEUROPE и т. д.)
			OUT_DEFAULT_PRECIS, // Точность вывода
			CLIP_DEFAULT_PRECIS, // Точность отсечения
			DEFAULT_QUALITY,   // Качество
			FF_SWISS,          // Семейство шрифта (FF_SWISS = "без засечек")
			L"Arial" // Имя шрифта 
		);
		if (hNew)
		{
			HFONT hOld = hFonthList1;
			hFonthList1 = hNew;
			if (hOld) DeleteObject(hOld);
		}

	} // меняем размер шрифтов элементов окна

	// Локальные помощники для масштабирования и округления 
	auto S = [&](int v) -> int {
		float r = v * Scale;
		return (int)(r >= 0.0f ? r + 0.5f : r - 0.5f);
		};

	// Позиции/размеры — масштабируем через S(), которое опирается на масштаб Scale,
	// а также применяем новый шрифт
	{
		{ // Раздел записи

			// Кнопки
			SetWindowPos(hBut1, NULL, S(59), H - S(192), S(75), S(24), SWP_NOZORDER);
			SetWindowPos(hBut2, NULL, S(154), H - S(192), S(75), S(24), SWP_NOZORDER);

			SendMessage(hBut1, WM_SETFONT, (WPARAM)hFontButton, TRUE);
			SendMessage(hBut2, WM_SETFONT, (WPARAM)hFontButton, TRUE);

			// Чекбокс "предварительная визуализация"
			SetWindowPos(hCheckBoxNoSave, NULL, S(8), H - S(164), S(272), S(20), SWP_NOZORDER);
			SendMessage(hCheckBoxNoSave, WM_SETFONT, (WPARAM)hFontComboBox, TRUE);

			// Выбор устройства записи (список)
			SetWindowPos(hComboBox5, NULL, S(8), H - S(140), S(272), S(95), SWP_NOZORDER);
			SendMessage(hComboBox5, WM_SETFONT, (WPARAM)hFontComboBox, TRUE);

			// Глубина звука (бит)
			SetWindowPos(hComboBox1, NULL, S(8), H - S(110), S(77), S(95), SWP_NOZORDER);
			SendMessage(hComboBox1, WM_SETFONT, (WPARAM)hFontComboBox, TRUE);

			// Частота дискретизации (Гц)
			SetWindowPos(hComboBox2, NULL, S(89), H - S(110), S(110), S(95), SWP_NOZORDER);
			SendMessage(hComboBox2, WM_SETFONT, (WPARAM)hFontComboBox, TRUE);

			// выбор МОНО-СТЕРЕО
			SetWindowPos(hComboBox3, NULL, S(203), H - S(110), S(77), S(95), SWP_NOZORDER);
			SendMessage(hComboBox3, WM_SETFONT, (WPARAM)hFontComboBox, TRUE);

			// Рамка раздела
			SetWindowPos(hGroupBox2, NULL, S(4), H - S(214), S(280), S(132), SWP_NOZORDER);
			SendMessage(hGroupBox2, WM_SETFONT, (WPARAM)hFontComboBox, TRUE);

		} // Раздел записи

		{ // Раздел воспроизведения

			// Кнопки
			SetWindowPos(hBut3, NULL, S(8), H - S(60), S(75), S(24), SWP_NOZORDER);
			SetWindowPos(hButPPause, NULL, S(87), H - S(60), S(114), S(24), SWP_NOZORDER);
			SetWindowPos(hBut5, NULL, S(205), H - S(60), S(75), S(24), SWP_NOZORDER);

			SendMessage(hBut3, WM_SETFONT, (WPARAM)hFontButton, TRUE);
			SendMessage(hButPPause, WM_SETFONT, (WPARAM)hFontButton, TRUE);
			SendMessage(hBut5, WM_SETFONT, (WPARAM)hFontButton, TRUE);

			// Рамка раздела
			SetWindowPos(hGroupBox3, NULL, S(4), H - S(80), S(280), S(52), SWP_NOZORDER);
			SendMessage(hGroupBox3, WM_SETFONT, (WPARAM)hFontComboBox, TRUE);

		} // Раздел воспроизведения

		{ // Элементы для обрезки звука

			// Трекбары выбора начала и конца для обрезки
			// Заметь: W — физическая ширина окна; добавляем логическое +6 и масштабируем
			SetWindowPos(hTrBar1, NULL, S(-3), H - S(255), W + S(6), S(20), SWP_NOZORDER);
			SetWindowPos(hTrBar2, NULL, S(-3), H - S(235), W + S(6), S(20), SWP_NOZORDER);

			// Кнопка "Обрезать звук"
			SetWindowPos(hButCutSound, NULL, W - S(134), H - S(214), S(130), S(24), SWP_NOZORDER);
			SendMessage(hButCutSound, WM_SETFONT, (WPARAM)hFontButton, TRUE);

		} // Элементы для обрезки звука

		{ // Список (для вывода диагностической информации) и управление им

			// Сам Список hList1
			SetWindowPos(hList1, NULL, S(288), H - S(186), S(292), S(158), SWP_NOZORDER);
			SendMessage(hList1, WM_SETFONT, (WPARAM)hFonthList1, TRUE);

			// Кнопка очистки hList1
			SetWindowPos(hBut4, NULL, S(510), H - S(24), S(70), S(20), SWP_NOZORDER);
			SendMessage(hBut4, WM_SETFONT, (WPARAM)hFontComboBox, TRUE);

			// Чекбокс для Вкл/выкл визуализации списка и кнопки очистки
			SetWindowPos(hCheckBoxShowMessageLog, NULL, S(288), H - S(24), S(200), S(20), SWP_NOZORDER);
			SendMessage(hCheckBoxShowMessageLog, WM_SETFONT, (WPARAM)hFontComboBox, TRUE);

		} // Список (для вывода диагностической информации) и управление им

		{ // прочие элементы окна

			// Вывод времени записи / воспроизведения (внизу слева)
			SetWindowPos(hWndTime, NULL, S(65), H - S(24), S(160), S(20), SWP_NOZORDER);
			SendMessage(hWndTime, WM_SETFONT, (WPARAM)hFontButton, TRUE);
			// SetWindowText(hWndTime, L" 00:00:00 / 00:00:00");

			// Вывод надписи "REC" внизу слева
			SetWindowPos(hWndRecSymbol, NULL, 4, H - 24, 60, 24, SWP_NOZORDER);

		} // прочие элементы окна

	} // применяем масштабирование и новый шрифт

	// Перерисовываем фоны groupbox'ов
	RedrawGroupBoxBackground(hGroupBox2, WhiteColorBrush);
	RedrawGroupBoxBackground(hGroupBox3, WhiteColorBrush);

}

BOOL ListBoxAddStr(HWND hLB, LPCWSTR AddStr, UINT MaxLength)
{
	if (!AddStr)
		return FALSE;

	if (hLB == NULL)
		return FALSE;

	// добавляем переданую строку в конец списка
	SendMessage(hLB, LB_ADDSTRING, 0, (LPARAM)AddStr);

	// Получаем количество элементов в ListBox 
	int itemCount = SendMessage(hLB, LB_GETCOUNT, 0, 0);

	if (itemCount > 0)
	{

		// если количество элементов больше максимально допустимого кол-ва
		while (itemCount > MaxLength)
		{
			SendMessage(hLB, LB_DELETESTRING, (WPARAM)0, 0L); // удаляем первую строку (начало) списка
			itemCount -= 1;
		}

		// Устанавливаем фокус на последний элемент
		SendMessage(hLB, LB_SETCARETINDEX, itemCount - 1, 0);

		return TRUE;
	}
	else
		return FALSE;

}

void RedrawGroupBoxBackground(HWND hwndGroupBox, HBRUSH hBachgroundBrush)
{
	if (!hwndGroupBox)
		return;

	if (!hBachgroundBrush)
		return;

	// Получаем HDC GroupBox
	HDC hdcGroupBox = GetDC(hwndGroupBox);

	// Получаем клиентскую область GroupBox
	RECT rcGroupBox;
	GetClientRect(hwndGroupBox, &rcGroupBox);

	FillRect(hdcGroupBox, &rcGroupBox, hBachgroundBrush); // Заливаем фон GroupBox кистью

	// в конце всегда свобождаем ресурсы
	ReleaseDC(hwndGroupBox, hdcGroupBox);

}

double convertSampleToDouble(const uint8_t* sampleBytes) 
{

	// Собираем 24-битное значение (младший порядок байт — Little Endian)
	int32_t sample = sampleBytes[0] | (sampleBytes[1] << 8) | (sampleBytes[2] << 16);

	// Проверяем знак (24 бита — знаковое число)
	if (sample & 0x800000) { // Если установлен знак
		sample |= 0xFF000000; // Расширяем до 32 бит, сохраняя знак
	}

	// Преобразуем в диапазон [-1.0, 1.0]
	return static_cast<double>(sample) / static_cast<double>(1 << 23); // Нормализация
}

void init()
{
	// Остальное
	Head_rec = NULL;// вначале список пуст

	RecBufferLenght = 0; // длина записанного фрагмента в байтах
	AudioTimeMS = 0;
	
	//структура с данными для графа звука
	mySoundGraph.Ready = false;
	mySoundGraph.Show = true; // по умолчанию стиль отображения "Граф звука", а не детальное
	mySoundGraph.Length = 40000; // пока такой размер, потом посмотрим
	mySoundGraph.DataL = new double [mySoundGraph.Length][2];
	mySoundGraph.DataR = new double[mySoundGraph.Length][2];

	// буферы записи пока пусты
	waveHdr[0].lpData = NULL;
	waveHdr[1].lpData = NULL;

	// структура формата записи по умолчанию (до считывания инфо с элементов окна)
	struct_rec_data.wFormatTag = WAVE_FORMAT_PCM; // записываем в стандартном WAVE
	struct_rec_data.nChannels = myChannels;// стерео
	struct_rec_data.nSamplesPerSec = mySamplesPerSec; // частота дискредитации
	struct_rec_data.wBitsPerSample = myBitsPerSample; // глубина звука (бит)
	struct_rec_data.nBlockAlign = struct_rec_data.wBitsPerSample/8*struct_rec_data.nChannels;
	struct_rec_data.cbSize = NULL;// доп. информация
	struct_rec_data.nAvgBytesPerSec = struct_rec_data.nSamplesPerSec * struct_rec_data.nBlockAlign;
	
	// структура формата проигрывания по умолчанию
	struct_play_data.wFormatTag = WAVE_FORMAT_PCM; // играем стандартный WAVE
	struct_play_data.nChannels = myChannels;// стерео
	struct_play_data.nSamplesPerSec = mySamplesPerSec; // частота дискредитации
	struct_play_data.wBitsPerSample = myBitsPerSample; // глубина звука (бит)
	struct_play_data.nBlockAlign = struct_play_data.wBitsPerSample/8*struct_play_data.nChannels;
	struct_play_data.cbSize = NULL;// доп. информация
	struct_play_data.nAvgBytesPerSec = struct_play_data.nSamplesPerSec * struct_play_data.nBlockAlign;

}

void ChangeFormatRec()
{
	// освобождаем память ранее выделенную под буферы записи
	delete []waveHdr[0].lpData;
	delete []waveHdr[1].lpData;
	waveHdr[0].lpData = NULL; // обнуляем ссылки
	waveHdr[1].lpData = NULL;

	//////////////////////////////////////////////////////////////////
	LRESULT return_result = 0; // храним возвращаемый результат

	// пытаемся считать из combobox данные о глубине звука
	return_result = SendMessage(hComboBox1,CB_GETCURSEL,0,0L);

	switch(return_result){
		case 0: // 8 бит
			myBitsPerSample = 8;
			break;
		case 1: // 16 бит
			myBitsPerSample = 16;
			break;
		case 2: // 24 бита (если не ошибки)
			myBitsPerSample = 24;
			break;
		default: // иначе 16 бит
			myBitsPerSample = 16;
	}

	// пытаемся считать с combobox данные о частоте дискретизации	
	return_result = SendMessage(hComboBox2,CB_GETCURSEL,0,0L);

	switch(return_result){
		case 0: // частота 8000
			mySamplesPerSec = 8000;
			break;
		case 1: // частота 11025
			mySamplesPerSec = 11025;
			break;
		case 2: // частота 22050
			mySamplesPerSec = 22050;
			break;
		case 3: // частота 44100
			mySamplesPerSec = 44100;
			break;
		case 4: // частота 48000
			mySamplesPerSec = 48000;
			break;
		case 5: // частота 96000
			mySamplesPerSec = 96000;
			break;
		case 6: // частота 192000
			mySamplesPerSec = 192000;
			break;

		default: // иначе частота 8000
			mySamplesPerSec = 8000;
	}

	// пытаемся считать с ComboBox данные о количестве каналов
	return_result = SendMessage(hComboBox3,CB_GETCURSEL,0,0L);

	switch(return_result){
		case 0: // МОНО
			myChannels = 1;
			break;
		case 1: 
			myChannels = 2; // СТЕРЕО1
			break;
		default: // иначе МОНО
			myChannels = 1;
	}

	// пытаемся считать с ComboBox5 данные об устройстве записи
	return_result = SendMessage(hComboBox5, CB_GETCURSEL, 0, 0L);
	if (return_result > 0)
		waveInDevNummer   = (UINT)return_result - 1; // если второй и дальше элемент списка
	else
		waveInDevNummer = WAVE_MAPPER; // в любой непонятной ситуации используем устр. по умолчанию

	ListBoxAddStr(hList1, L"Используем устр. записи:");
	WCHAR* cBuf1 = new WCHAR[150];
	SendMessage(hComboBox5, CB_GETLBTEXT, return_result,(LPARAM)cBuf1); // записываем инфу с выбраного поля hComboBox5
	ListBoxAddStr(hList1, cBuf1); // выводим эту инфу в окно hList1
	delete []cBuf1;

  // перезаполняем структуры записи и воспроизведения
	// структура формата записи
	struct_rec_data.wFormatTag = WAVE_FORMAT_PCM; // записываем в стандартном WAVE
	struct_rec_data.nChannels = myChannels;// стерео
	struct_rec_data.nSamplesPerSec = mySamplesPerSec; // частота дискретизации
	struct_rec_data.wBitsPerSample = myBitsPerSample; // глубина звука (бит)
	struct_rec_data.nBlockAlign = struct_rec_data.wBitsPerSample/8*struct_rec_data.nChannels;
	struct_rec_data.cbSize = NULL;// доп. информация
	struct_rec_data.nAvgBytesPerSec = struct_rec_data.nSamplesPerSec * struct_rec_data.nBlockAlign;
	
	// структура формата проигрывания
	struct_play_data.wFormatTag = WAVE_FORMAT_PCM; // играем стандартный WAVE
	struct_play_data.nChannels = myChannels;// стерео
	struct_play_data.nSamplesPerSec = mySamplesPerSec; // частота дискретизации
	struct_play_data.wBitsPerSample = myBitsPerSample; // глубина звука (бит)
	struct_play_data.nBlockAlign = struct_play_data.wBitsPerSample/8*struct_play_data.nChannels;
	struct_play_data.cbSize = NULL;// доп. информация
	struct_play_data.nAvgBytesPerSec = struct_play_data.nSamplesPerSec * struct_play_data.nBlockAlign;
	
	// размер буфера равен примерно 1/20 секунды (8бит * 20 = 160 в знаменателе)
		switch(mySamplesPerSec){
		case 11025: // 19 буферов в 1 секунде
			::BufferSize = 11040*myChannels*myBitsPerSample/160;
			break;
		case 22050: // 19 буферов в 1 секунде
			::BufferSize = 22060*myChannels*myBitsPerSample/160;
			break;
		default:  // а тут 20 буферов в 1 секунде (для всех прочих случаев)
			::BufferSize = mySamplesPerSec*myChannels*myBitsPerSample/160;
	}
	ListBoxAddStr(hList1, L"Форматы записи и воспроизв. изменены!");
}

bool glVichMinMaxXY(rec_list* Head) 
{

	// находим границы высоты (ось Y) в зависимости от глубыны звука
	switch (myBitsPerSample)
	{
	case 8:
		::glYmin = -128;
		::glYmax = 127;
		break;
	case 16:
		::glYmin = -32768;
		::glYmax = 32767;
		break;
	case 24:
		::glYmin = -1;
		::glYmax = 1;
		break;
	default:
		return 0;
	}

	// если визуализируем только текущий буфер в режиме реального времени
	if(Head == NULL){

		if (::myBitsPerSample == 8) // если глубина звука 8 бит то в буфере такое же количество отсчётов 
		{ 
			if (::myChannels == 1)
				::glXmax = BufferSize;
			if (::myChannels == 2)
				::glXmax = BufferSize / 2;
		}

		if(::myBitsPerSample == 16) // если глубина звука 16 то в буфере в 2 или 4 раза меньше отсчётов
		{ 
			if (::myChannels == 1)
				::glXmax = BufferSize / 2;
			if(::myChannels == 2)
				::glXmax = BufferSize / 4;
		}

		if (::myBitsPerSample == 24) // если глубина звука 24 то в буфере в 3 или 6 раз меньше отсчётов
		{
			if (::myChannels == 1)
				::glXmax = BufferSize / 3;
			if (::myChannels == 2)
				::glXmax = BufferSize / 6;
		}

		::glXmin = 0; // отсчёт идет всегда с 0

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		double glProcentX, glProcentY;
		glProcentX = (glXmax-glXmin)/500;
		glProcentY = (glYmax-glYmin)/500;
		gluOrtho2D(glXmin-glProcentX,glXmax+glProcentX,
			glYmin-glProcentY,glYmax+glProcentY); // размерность поля рисования (координатной оси XY)
		
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity(); // обнуляем матрицу модельных преобразований

		return TRUE; // успешное завершение
	} // если визуализируем только текущий буфер в режиме реального времени

	// иначе визуализируем всю запись
	// находим количество выборок (за всё время) - X
	// теперь находим сколько в одном буфере данных
	WORD myAvgBytesPerSec = ::myChannels*::myBitsPerSample/8; // количество каналов* кол. байт в одном такте
	::glXmax = ::RecBufferLenght/(double)myAvgBytesPerSec; // максимальное количество по оси X
	::glXmin = 0;// начало координат по оси X

	// выведем в редактор найденные данные в окно вывода
	WCHAR s2[20];
	WCHAR s1[120];
	wcscpy(s1, L"OpenGL: X(");
	swprintf(s2, 20, L"%.0f", ::glXmin);
	wcscat(s1,s2);
	wcscat(s1,L", ");
	swprintf(s2, 20, L"%.0f", ::glXmax);
	wcscat(s1,s2);
	wcscat(s1,L"), Y(");
	swprintf(s2, 20, L"%.0f", ::glYmin);
	wcscat(s1,s2);
	wcscat(s1,L", ");
	swprintf(s2, 20, L"%.0f", ::glYmax);
	wcscat(s1,s2);
	wcscat(s1,L")");

	ListBoxAddStr(hList1, s1);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	double glProcentX, glProcentY;
	  glProcentX = (glXmax-glXmin)/500;
	  glProcentY = (glYmax-glYmin)/500;
    gluOrtho2D(glXmin-glProcentX,glXmax+glProcentX,
	  glYmin-glProcentY,glYmax+glProcentY); // размеры поля рисования немного больше чем надо

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity(); // обнуляем матрицу модельных преобразований


	return TRUE; // успешное завершение
}

int SetWindowPixelFormat()
{
	int m_GLPixelIndex;
	PIXELFORMATDESCRIPTOR pfd;

	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW |
		PFD_DOUBLEBUFFER | // двойная буферизация
		PFD_SUPPORT_OPENGL;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cRedBits = 8;
	pfd.cRedShift = 16;
	pfd.cGreenBits = 8;
	pfd.cGreenShift = 8;
	pfd.cBlueBits = 8;
	pfd.cBlueShift = 0;
	pfd.cAlphaBits = 0;
	pfd.cAlphaShift = 0;
	pfd.cAccumBits = 64;
	pfd.cAccumRedBits = 16;
	pfd.cAccumGreenBits = 16;
	pfd.cAccumBlueBits = 16;
	pfd.cAccumAlphaBits = 0;
	pfd.cDepthBits = 32;
	pfd.cStencilBits = 8;
	pfd.cAuxBuffers = 0;
	pfd.iLayerType = PFD_MAIN_PLANE;
	pfd.bReserved = 0;
	pfd.dwLayerMask = 0;
	pfd.dwVisibleMask = 0;
	pfd.dwDamageMask = 0;


	m_GLPixelIndex = ChoosePixelFormat(hDC, &pfd);
	if (m_GLPixelIndex == 0) // Let's choose a default index.
	{
		m_GLPixelIndex = 1;
		if (DescribePixelFormat(hDC, m_GLPixelIndex, sizeof(PIXELFORMATDESCRIPTOR), &pfd) == 0)
			return 0;
	}


	if (SetPixelFormat(hDC, m_GLPixelIndex, &pfd) == FALSE)
		return 0;

	return 1;
}

void reshape(int x, int y, int width, int height)
{

	  // размера и расположение порта вывода OpenGL внутри главного окна
      glViewport(x,y,width,height); 

      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();

	  double glProcentX, glProcentY;
	  glProcentX = (glXmax-glXmin)/500; //сделать размер области отображения на 0.2 процент шире макс
	  glProcentY = (glYmax-glYmin)/500; //сделать размер области отображения на 0.2 процент выше макс
	  // размеры поля рисования
	  gluOrtho2D(glXmin-glProcentX,glXmax+glProcentX,glYmin-glProcentY,glYmax+glProcentY);

	  glMatrixMode(GL_MODELVIEW);
      glLoadIdentity(); // обнуляем матрицу модельных преобразований

}

void DrawHorizontalLine(double XStart_gl, double XEnd_gl, double Y_gl, double Width_px)
{
	// Рисует горизонтальную полосу, где X и Y координаты
	// заданы во внутренних координатах OpenGL, 
	// а толщина задаётся в пикселях (Width_px). Половина толщины над Y, половина под Y.

	// защита
	if (Width_px <= 0.0)
		return;
	if (::glViewportWidth <= 0 || ::glViewportHeight <= 0)
		return;

	// Перевод пикселей толщины -> величина в GL-координатах по Y
	// 1 пиксель по вертикали = (glYmax - glYmin) / viewportHeight
	double glPerPixelY = (::glYmax - ::glYmin) / (double)::glViewportHeight;
	double halfGL = (Width_px * 0.5) * glPerPixelY;

	// вершины прямоугольника в GL-координатах
	double vx1 = XStart_gl;        double vy1 = Y_gl + halfGL; // начало верх
	double vx2 = XStart_gl;        double vy2 = Y_gl - halfGL; // начало низ
	double vx3 = XEnd_gl;          double vy3 = Y_gl - halfGL; // конец низ
	double vx4 = XEnd_gl;          double vy4 = Y_gl + halfGL; // конец верх

	// Рисуем двумя треугольниками (надёжно для legacy)
	glBegin(GL_TRIANGLES);

	glVertex2d(vx1, vy1);
	glVertex2d(vx2, vy2);
	glVertex2d(vx3, vy3);

	glVertex2d(vx3, vy3);
	glVertex2d(vx4, vy4);
	glVertex2d(vx1, vy1);

	glEnd();

}

void DrawQuadPoint(double X_gl, double Y_gl, double Size_px)
{
	// Рисует квадрат, где X и Y координаты его центра
    // и заданы во внутренних координатах OpenGL,
	// а длина грани задаётся в пикселях (Size_px).
	// Аналог glPointSize(Size_px) + glBegin(GL_POINTS)

	// начальные проверки
	if (Size_px <= 0.0)
		return;
	if (::glViewportWidth <= 0 || ::glViewportHeight <= 0)
		return;

	// сколько GL-единиц по X и Y соответствует 1 пикселю
	double glPerPixelX = (::glXmax - ::glXmin) / (double)::glViewportWidth;
	double glPerPixelY = (::glYmax - ::glYmin) / (double)::glViewportHeight;

	// половина размера в GL-координатах
	double halfGLx = (Size_px * 0.5) * glPerPixelX;
	double halfGLy = (Size_px * 0.5) * glPerPixelY;

	// вершины нашего квадрата
	double x1 = X_gl - halfGLx; double y1 = Y_gl - halfGLy; // низ левый
	double x2 = X_gl + halfGLx; double y2 = Y_gl - halfGLy; // низ правый
	double x3 = X_gl + halfGLx; double y3 = Y_gl + halfGLy; // верх правый
	double x4 = X_gl - halfGLx; double y4 = Y_gl + halfGLy; // верх левый

	// рисуем двумя треугольниками (надёжно для legacy)
	glBegin(GL_TRIANGLES);

	glVertex2d(x1, y1);
	glVertex2d(x2, y2);
	glVertex2d(x3, y3);

	glVertex2d(x3, y3);
	glVertex2d(x4, y4);
	glVertex2d(x1, y1);

	glEnd();

}

void display(double localPlayBufferPos)
{
	// очистим окно вывода OpenGL (только заданую область)
	glClearColor(0.97, 0.99, 1, 1); // цвет очистки области OpenGL
	glScissor(glViewportX, glViewportY, glViewportWidth, glViewportHeight); // границы очистки области
	glEnable(GL_SCISSOR_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_SCISSOR_TEST);

	//  если воспроизводим текущий звук (отображение текущего воспр. буфера + прогрессбар)
	if (isPlayThreadRunning.load() == true)
	{
		// получим ссылку на буфер, который необходимо отобразить, и атомарной переменной
		rec_list* Local_Last_rec = Play_Last_rec.load(std::memory_order_acquire);

		// отображаем график последнего буфера звука

		if (myChannels == 1) // данные о звуке заполнены и звук моно
		{
			// отображаем график моно звука
			ShowLastBufMono(Local_Last_rec, DetailedShow); // визуализируем без перегрузки
			// ShowLastBufMonoOverload(Local_Last_rec); // визуализируем также перегрузку

			// отображение Peak meter
			if (ShowOverload == TRUE)
				ShowPeakMeterMono(Local_Last_rec);

			// отображение VU meter
			if (ShowVUMeter == TRUE)
				ShowVUMeterMono(Local_Last_rec);
		
		} // данные о звуке заполнены и звук моно

		if (myChannels == 2) // данные о звуке заполнены и звук стерео
		{			
			// отображаем график стерео звука
			ShowLastBufStereo(Local_Last_rec, DetailedShow); // визуализируем без перегрузки
			// ShowLastBufStereoOverload(Local_Last_rec); // визуализируем также перегрузку
			
			// отображение Peak meter
			if (ShowOverload == TRUE)
				ShowPeakMeterStereo(Local_Last_rec);

			// отображение VU meter
			if (ShowVUMeter == TRUE)
				ShowVUMeterStereo(Local_Last_rec);
		
		} // данные о звуке заполнены и звук стерео


		// рисуем шкалу воспроизведения звука (трекбар)

		double xPlayPos, yPlayPos;

		xPlayPos = (::glXmax - ::glXmin) * (localPlayBufferPos / ::RecBufferLenght) + ::glXmin;

		yPlayPos = 4.0 / (double)::glViewportHeight;  // от 0 до 1 (4 точки вверх внутри области OpenGL)
		yPlayPos = yPlayPos * (glYmax - glYmin) + glYmin;

		glLineWidth(8); // ширина полосы воспроизведения

		// Включаем blending (прозрачность) и задаём функцию смешивания
		// Сохраняем текущие состояния
		// GLint blendSrc, blendDst;
		// glGetIntegerv(GL_BLEND_SRC, &blendSrc);
		// glGetIntegerv(GL_BLEND_DST, &blendDst);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glBegin(GL_LINES);
		glColor4d(0.6, 0.3, 0.7, 0.5); // полоса воспроизведения фиолетовая (полупрозрачная)
		glVertex2d(::glXmin, yPlayPos);
		glColor4d(0.4, 0.1, 0.5, 0.5); // полоса воспроизведения фиолетовая (полупрозрачная), градиент
		glVertex2d(xPlayPos, yPlayPos);
		glEnd();

		// Возвращаем blending (прозрачность) в исходное состояние
		// glBlendFunc(blendSrc, blendDst);
		glDisable(GL_BLEND);

		glLineWidth(1); // далее опять всё рисуем стандартной толщиной


		// далее рисуем окантовку окошка воспроизведения
		glBegin(GL_LINE_LOOP);
		glColor3d(0.1, 0.1, 0.1); // цвет отделяющей линии
		glVertex2d(glXmin, glYmin);
		glVertex2d(glXmin, glYmax);
		glVertex2d(glXmax, glYmax);
		glVertex2d(glXmax, glYmin);
		glEnd();

		glFinish(); // немедленный вывод
		SwapBuffers(wglGetCurrentDC());

		return;
	
	} //  если воспроизводим текущий звук

	// если записываем звук (отображение последнего записанного буфера)
	if (isRecThreadRunning.load() == true)
	{

		Last_rec_Mutex.lock(); // блокируем использование Rec_Last_rec в других потоках

		rec_list* Local_Last_rec = Rec_Last_rec;

		Last_rec_Mutex.unlock(); // разблокируем использование Rec_Last_rec в других потоках

		if (Local_Last_rec != NULL)
		{

			// отображаем график последнего буфера

			if (myChannels == 1) // звук моно
			{
				// отображаем график моно звука
				ShowLastBufMono(Local_Last_rec, DetailedShow); // визуализируем без перегрузки
				// ShowLastBufMonoOverload(Local_Last_rec); // визуализируем также перегрузку
				
				// отображение Peak meter
				if (ShowOverload == TRUE)
					ShowPeakMeterMono(Local_Last_rec);

				// отображение VU meter
				if (ShowVUMeter == TRUE)
					ShowVUMeterMono(Local_Last_rec);

			} // звук моно

			if (myChannels == 2) // звук стерео
			{
				// отображаем график стерео звука
				ShowLastBufStereo(Local_Last_rec, DetailedShow); // визуализируем без перегрузки
				// ShowLastBufStereoOverload(Local_Last_rec); // визуализируем также перегрузку
				
				// отображение Peak meter
				if (ShowOverload == TRUE)
					ShowPeakMeterStereo(Local_Last_rec);		

				// отображение VU meter
				if (ShowVUMeter == TRUE)
					ShowVUMeterStereo(Local_Last_rec);

			} // звук стерео
		
		} // if (Local_Last_rec != NULL)
		
		// далее рисуем окантовку окошка воспроизведения

		glBegin(GL_LINE_LOOP);
		glColor3d(0.1, 0.1, 0.1); // цвет отделяющей линии
		glVertex2d(glXmin, glYmin);
		glVertex2d(glXmin, glYmax);
		glVertex2d(glXmax, glYmax);
		glVertex2d(glXmax, glYmin);
		glEnd();

		glFinish(); // немедленный вывод
		SwapBuffers(wglGetCurrentDC());

		return;

	} // если записываем звук

	// иначе перерисовываем график всего аудио
	else
	{
		// если разрешено визуализировать график всего звука
		if (ShowAllSound)
		{
			// отображаем график моно звука
			if (myChannels == 1) 
			{
				
				// если визуализация массива минимумов-максимумов
				if (mySoundGraph.Show && mySoundGraph.Ready)
				{
					ShowSoundGraph(showPeaks);
				}

				else // иначе детальная визуализация каждой точки звука
				{
					ShowAllBufMono(showPeaks);
				}
					
			} // Sound == mono

			// отображаем графики стерео звука
			if (myChannels == 2) 
			{
				// если визуализация массива минимумов-максимумов
				if (mySoundGraph.Show && mySoundGraph.Ready)
				{
					ShowSoundGraph(showPeaks);
				}

				else // иначе детальная визуализация каждой точки звука
				{
					ShowAllBufStereo(showPeaks);
				}
			} // Sound == stereo

			// далее рисуем окантовку окошка воспроизведения
			glBegin(GL_LINE_LOOP);
			glColor3d(0.1, 0.1, 0.1); // цвет отделяющей линии
			glVertex2d(glXmin, glYmin);
			glVertex2d(glXmin, glYmax);
			glVertex2d(glXmax, glYmax);
			glVertex2d(glXmax, glYmin);
			glEnd();

			glFinish(); // немедленный вывод
			SwapBuffers(wglGetCurrentDC());

			return;

		}// if (ShowAllSound == TRUE)

		// если запрещено визуализировать график всего звука
		// рисуем только окантовку области OpenGL
		else  // if (ShowAllSound == FALSE)
		{
			// рисуем окантовку окошка воспроизведения
			glBegin(GL_LINE_LOOP);
			glColor3d(0.1, 0.1, 0.1); // цвет отделяющей линии
			glVertex2d(glXmin, glYmin);
			glVertex2d(glXmin, glYmax);
			glVertex2d(glXmax, glYmax);
			glVertex2d(glXmax, glYmin);
			glEnd();

			glFinish(); // немедленный вывод
			SwapBuffers(wglGetCurrentDC());

			return;

		} // if (ShowAllSound == FALSE)

	} // перерисовывка графика всего аудио
  

  //glFinish();
}

void ShowAllBufStereo(BOOL localShowPeaks)
{
	// начальные проверки
	if (myChannels != 2)
		return;

	if (Head_rec == NULL)
		return;

	/////////////////////////////////////////////////////////////////////////////
	// делаем мини окошко "Визуализация графика звука..."
	HWND hWndWaitWindow = NULL;
	// Если длина аудио более 5 мин. (для СТЕРЕО, 44КГц, 16 бит)
	if (::RecBufferLenght > 52920000.0)
	{
		// определяем текущий масштаб в системе
		float Scale = (float)WindowDPI / 96.0f; // масштаб окна (в Win XP равен 1.0)

		int widthWaitWindow = int(250 * Scale); // ширина нашего мини окна
		int heightWaitWindow = int(80 * Scale); // высота нашего мини окна

		// получаем размеры и координаты главного окна
		RECT rcParent;
		GetWindowRect(hWnd, &rcParent);

		// мини окно всегда будет в центре главного окна
		int xWaitWindow = rcParent.left + (rcParent.right - rcParent.left - widthWaitWindow) / 2;
		int yWaitWindow = rcParent.top + (rcParent.bottom - rcParent.top - heightWaitWindow) / 2;

		// создаём мини окно
		hWndWaitWindow = CreateWindowEx(
			WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
			L"STATIC",
			L"Визуализация графика звука...",
			WS_POPUP | WS_BORDER | WS_VISIBLE | SS_CENTER,
			xWaitWindow, yWaitWindow, widthWaitWindow, heightWaitWindow,
			hWnd, NULL, hInst, NULL);

		// меняем стиль шрифта мини окна
		SendMessage(hWndWaitWindow, WM_SETFONT, (WPARAM)hFontButton, TRUE);

		UpdateWindow(hWndWaitWindow);

		// ... и блокируем ввод в главное окно (пока идёт обработка данных
		EnableWindow(hWnd, FALSE);

	}
	/////////////////////////////////////////////////////////////////////////////


	int stepSize = 1; // сколько позиций при визуализаци пропускаем (1 = каждая точка)
	
	double yPos; // позиция по оси Y для текущего отсчёта
	double i, j;

	if (myBitsPerSample == 8) // глубина звука 8 бит
	{
		rec_list* Tek_buf = ::Head_rec;
		unsigned char* tek_pos;

		// левый канал
		glColor3d(0.0, 0.55, 0.0);
		glBegin(GL_LINE_STRIP);
		for (i = 0; i < ::glXmax;)
		{
			tek_pos = (unsigned char*)Tek_buf->data;

			// для каждого буфера смотрим не вышли ли за его предел!
			for (j = 0; j < ::BufferSize / 2; j += stepSize)
			{
				yPos = (double)*tek_pos;
				yPos = (yPos - 128.0) / 2 + 64.0; // смещаем график вверх!
				glVertex2d(i + j, yPos);
				tek_pos = tek_pos + 2 * stepSize; // 2 канала * 1 байт (8 бит звук)
			}
			i += BufferSize / 2; // в стерео буфере с глубиной 8 бит в 2 раза меньше отсчётов
			Tek_buf = Tek_buf->next;
		}
		glEnd();

		// правый канал
		Tek_buf = ::Head_rec; // опять возвращаемся в начало записи

		glColor3d(0.0, 0.0, 0.65);
		glBegin(GL_LINE_STRIP);
		for (i = 0; i < ::glXmax;)
		{
			tek_pos = (unsigned char*)Tek_buf->data;
			tek_pos += 1; // правый канал смещён на 1 байт относительно начала буфера

			// для каждого буфера смотрим не вышли ли за его предел!
			for (j = 0; j < ::BufferSize / 2; j += stepSize)
			{
				yPos = (double)*tek_pos;
				yPos = (yPos - 128.0) / 2 - 64.0; // смещаем график вниз!
				glVertex2d(i + j, yPos);
				tek_pos = tek_pos + 2 * stepSize; // 2 канала * 1 байт (8 бит звук)
			}
			i += BufferSize / 2; // в стерео буфере с глубиной 8 бит в 2 раза меньше отсчётов
			Tek_buf = Tek_buf->next;
		}
		glEnd();

	}// if (myBitsPerSample == 8)

	if (myBitsPerSample == 16) // глубина звука 16 бит
	{
		rec_list *Tek_buf = ::Head_rec;
		short *tek_pos;

		// левый канал
		glColor3d(0.0, 0.55, 0.0);
		glBegin(GL_LINE_STRIP);
		for(i=0; i<::glXmax;)
		{
			tek_pos = (short*)Tek_buf->data;

			// для каждого буфера смотрим не вышли ли за его предел!
			for (j = 0; j < ::BufferSize / 4; j += stepSize)
			{
				yPos = (double)*tek_pos / 2 + 16383.5; // смещаем в верхнюю часть области визуализации
				glVertex2d(i + j, yPos);
				tek_pos = tek_pos + 2 * stepSize; // 2 канала (+ указатель tek_pos - 2 байта)
			}
			i += BufferSize/4;// в стерео буфере с глубиной 16 бит в 4 раза меньше отсчётов
			Tek_buf = Tek_buf->next;
		}
		glEnd();

		// правый канал
		Tek_buf = ::Head_rec; // опять возвращаемся в начало записи

		glColor3d(0.0, 0.0, 0.65);
		glBegin(GL_LINE_STRIP);
		for(i=0; i<::glXmax;)
		{
			tek_pos = (short*)Tek_buf->data;
			tek_pos += 1; // правый канал смещён на +1 (2 байта) относительно начала буфера

			// для каждого буфера смотрим не вышли ли за его предел!
			for (j = 0; j < ::BufferSize / 4; j += stepSize)
			{
				yPos = (double)*tek_pos / 2 - 16383.5; // смещаем в нижнюю часть области визуализации
				glVertex2d(i + j, yPos);
				tek_pos = tek_pos + 2 * stepSize; // 2 канала (+ указатель tek_pos - 2 байта)
			}
			i += BufferSize/4;// в стерео буфере с глубиной 16 бит в 4 раза меньше отсчётов
			Tek_buf = Tek_buf->next;
		}
		glEnd();

	} // if (myBitsPerSample == 16)

	if (myBitsPerSample == 24) // глубина звука 24 бита
	{
		rec_list* Tek_buf = ::Head_rec;
		unsigned char* tek_pos;

		// левый канал
		glColor3d(0.0, 0.55, 0.0);
		glBegin(GL_LINE_STRIP);
		for (i = 0; i < ::glXmax;)
		{
			tek_pos = (unsigned char*)Tek_buf->data;

			// для каждого буфера смотрим не вышли ли за его предел!
			for (j = 0; j < ::BufferSize / 6; j += stepSize)
			{
				yPos = convertSampleToDouble(tek_pos);
				yPos = yPos / 2 + 0.5; // смещаем в верхнюю часть области визуализации
				glVertex2d(i + j, yPos);
				tek_pos = tek_pos + 6 * stepSize; // 2 канала * 3 байта (24 бит звук)
			}
			i += BufferSize / 6;// в стерео буфере с глубиной 24 бит в 6 раз меньше отсчётов
			Tek_buf = Tek_buf->next;
		}
		glEnd();

		// правый канал
		Tek_buf = ::Head_rec; // опять возвращаемся в начало записи

		glColor3d(0.0, 0.0, 0.65);
		glBegin(GL_LINE_STRIP);
		for (i = 0; i < ::glXmax;)
		{
			tek_pos = (unsigned char*)Tek_buf->data;
			tek_pos += 3; // правый канал смещён на +3 байта относительно начала буфера

			// для каждого буфера смотрим не вышли ли за его предел!
			for (j = 0; j < ::BufferSize / 6; j += stepSize)
			{
				yPos = convertSampleToDouble(tek_pos);
				yPos = yPos / 2 - 0.5; // смещаем в нижнюю часть области визуализации
				glVertex2d(i + j, yPos);
				tek_pos = tek_pos + 6 * stepSize; // 2 канала * 3 байта (24 бит звук)
			}
			i += BufferSize / 6;// в стерео буфере с глубиной 24 бит в 6 раз меньше отсчётов
			Tek_buf = Tek_buf->next;
		}
		glEnd();

	} // if (myBitsPerSample == 24)

	// если нужно, отображаем пики/перегрузку
	if (localShowPeaks == TRUE)
	{
		// мин и макс значения для 8, 16, 24 битного звука
		double minY, maxY;

		// порог срабатывания перегрузки для 8, 16, 24 битного звука (99.99% от мин/макс)
		double minYThreshold, maxYThreshold;

		double xPos; // буферная переменная, позиция по оси Х

		glColor3d(1.0, 0.2, 0.2); // цвет для линий пиков/перегрузки (красный)

		// глубина звука 8 бит
		if (myBitsPerSample == 8)
		{
			minY = -128.0;
			maxY = 127.0;

			minYThreshold = -127.99;
			maxYThreshold = 126.99;

			// для всего звука ищем пики/перегрузку и отображаем
			glBegin(GL_LINES);

			rec_list* Tek_buf = Head_rec;
			unsigned char* tek_pos;

			xPos = 0; // позиция по оси Х

			// для всех хвуковых данных, пока не дошли до конца
			while (Tek_buf != NULL)
			{
				tek_pos = (unsigned char*)Tek_buf->data;

				// для каждого буфера смотрим не вышли ли за его предел
				for (j = 0; j < ::BufferSize; j += 2)
				{
					// левый канал
					yPos = (double)*tek_pos;
					yPos = yPos - 128.0;

					if (yPos > maxYThreshold || yPos < minYThreshold)
					{
						// координаты вертикальной линии перегрузки
						// для левого канала (в верхней части)
						glVertex2d(xPos, 0.0);
						glVertex2d(xPos, maxY);
					}
					
					tek_pos += 1; // к следующему звуковому отсчёту (правый канал)

					// правый канал
					yPos = (double)*tek_pos;
					yPos = yPos - 128.0;

					if (yPos > maxYThreshold || yPos < minYThreshold)
					{
						// координаты вертикальной линии перегрузки
						// для правого канала (в нижней части)
						glVertex2d(xPos, minY);
						glVertex2d(xPos, 0);
					}

					tek_pos += 1; // к следующему звуковому отсчёту

					xPos += 1; // позиция по оси X тоже сместилась вправо

				}

				Tek_buf = Tek_buf->next;

			} // для всех хвуковых данных, пока не дошли до конца

			glEnd();

		} // if (myBitsPerSample == 8)

		// глубина звука 16 бит
		if (myBitsPerSample == 16)
		{
			minY = -32768.0;
			maxY = 32767.0;

			minYThreshold = -32764.0;
			maxYThreshold = 32763.0;

			// для всего звука ищем пики/перегрузку и отображаем
			glBegin(GL_LINES);

			rec_list* Tek_buf = Head_rec;
			short* tek_pos;

			xPos = 0; // позиция по оси Х

			// для всех хвуковых данных, пока не дошли до конца
			while (Tek_buf != NULL)
			{
				tek_pos = (short*)Tek_buf->data;

				// для каждого буфера смотрим не вышли ли за его предел
				for (j = 0; j < ::BufferSize; j += 4)
				{
					// левый канал
					yPos = (double)*tek_pos;

					if (yPos > maxYThreshold || yPos < minYThreshold)
					{
						// координаты вертикальной линии перегрузки
						// для левого канала (в верхней части)
						glVertex2d(xPos, 0.0);
						glVertex2d(xPos, maxY);
					}

					tek_pos += 1; // к следующему звуковому отсчёту (правый канал) +2 байта

					// правый канал
					yPos = (double)*tek_pos;

					if (yPos > maxYThreshold || yPos < minYThreshold)
					{
						// координаты вертикальной линии перегрузки
						// для правого канала (в нижней части)
						glVertex2d(xPos, minY);
						glVertex2d(xPos, 0);
					}

					tek_pos += 1; // к следующему звуковому отсчёту +2 байта

					xPos += 1; // позиция по оси X тоже сместилась вправо

				}

				Tek_buf = Tek_buf->next;

			} // для всех хвуковых данных, пока не дошли до конца

			glEnd();

		} // if (myBitsPerSample == 16)

		// глубина звука 24 бита
		if (myBitsPerSample == 24)
		{
			minY = -1.0;
			maxY = 1.0;

			minYThreshold = -0.9999;
			maxYThreshold = 0.9999;

			// для всего звука ищем пики/перегрузку и отображаем
			glBegin(GL_LINES);

			rec_list* Tek_buf = Head_rec;
			unsigned char* tek_pos;

			xPos = 0; // позиция по оси Х

			// для всех хвуковых данных, пока не дошли до конца
			while (Tek_buf != NULL)
			{
				tek_pos = (unsigned char*)Tek_buf->data;

				// для каждого буфера смотрим не вышли ли за его предел
				for (j = 0; j < ::BufferSize; j += 6)
				{
					// левый канал
					yPos = convertSampleToDouble(tek_pos);

					if (yPos > maxYThreshold || yPos < minYThreshold)
					{
						// координаты вертикальной линии перегрузки
						// для левого канала (в верхней части)
						glVertex2d(xPos, 0.0);
						glVertex2d(xPos, maxY);
					}

					tek_pos += 3; // к следующему звуковому отсчёту (правый канал) +3 байта

					// правый канал
					yPos = convertSampleToDouble(tek_pos);

					if (yPos > maxYThreshold || yPos < minYThreshold)
					{
						// координаты вертикальной линии перегрузки
						// для правого канала (в нижней части)
						glVertex2d(xPos, minY);
						glVertex2d(xPos, 0);
					}

					tek_pos += 3; // к следующему звуковому отсчёту +3 байта

					xPos += 1; // позиция по оси X тоже сместилась вправо

				}

				Tek_buf = Tek_buf->next;

			} // для всех хвуковых данных, пока не дошли до конца

			glEnd();

		} // if (myBitsPerSample == 24)

	
	} // if (localShowPeaks == TRUE)


	/////////////////////////////////////////////////////////////////////////////
	// если выводили, то убираем мини окошко "Визуализация графика звука..."
	if (hWndWaitWindow != NULL)
	{
		DestroyWindow(hWndWaitWindow);
		// ...снова активируем главное окно
		EnableWindow(hWnd, TRUE);
		// и возвращаем фокус главному окну
		SetForegroundWindow(hWnd);
	}
	/////////////////////////////////////////////////////////////////////////////
	
}

void ShowLastBufStereo(rec_list *Buffer, BOOL ShowAllPoints)
{
	if (myChannels != 2)
		return;

	ULONG StepSize;
	if (ShowAllPoints)
		StepSize = 1; // визуализируем каждую точку буфера
	else
		StepSize = 2; // визуализируем через одну точку буфера
	
	double yPos;// позиция по оси Y при отображении текущей точки
	
	ULONG j;

	// глубина звука 8 бит
	if (myBitsPerSample == 8)
	{

		unsigned char* tek_pos = (unsigned char*) Buffer->data;

		// левый канал
		{
			glColor3d(0.0, 0.6, 0.0); // цвет графика звука левый канал

			glBegin(GL_LINE_STRIP);

			// для буфера смотрим не вышли ли за его предел
			for (j = 0; j < ::BufferSize / 2; j += StepSize)
			{
				yPos = (double)*tek_pos;
				yPos = (yPos - 128.0) / 2.0 + 64.0; // смещаем в верхнюю часть области визуализации

				glVertex2d((double)j, yPos);

				tek_pos += 2 * StepSize; // так как в отсчёте 2 канала по 8 бит или 1 байту
			}

			glEnd();
		
		} // левый канал

		tek_pos = (unsigned char*) Buffer->data;
		tek_pos++; //правый канал смещён на +1 байт относительно начала буфера

		// правый канал
		{
			glColor3d(0.0, 0.0, 0.7); // цвет графика звука правый канал

			glBegin(GL_LINE_STRIP);

			// для буфера смотрим не вышли ли за его предел
			for (j = 0; j < ::BufferSize / 2; j += StepSize)
			{
				yPos = (double)*tek_pos;
				yPos = (yPos - 128.0) / 2.0 - 64.0; // смещаем в нижнюю часть области визуализации

				glVertex2d((double)j, yPos);

				tek_pos += 2 * StepSize; // так как в отсчёте 2 канала по 8 бит или 1 байту
			}

			glEnd();

		} // правый канал

	} // глубина звука 8 бит

	// глубина звука 16 бит
	if (myBitsPerSample == 16)
	{		
		short* tek_pos = (short*)Buffer->data;

		// левый канал
		{
			glColor3d(0.0, 0.6, 0.0); // цвет графика звука левый канал

			glBegin(GL_LINE_STRIP);

			// для  буфера смотрим не вышли ли за его предел
			for (j = 0; j < ::BufferSize / 4; j += StepSize)
			{
				yPos = (double)*tek_pos / 2.0 + 16383.5; // смещаем в верхнюю часть области визуализации

				glVertex2d((double)j, yPos);

				tek_pos += 2 * StepSize; // тип short 2 байта, а затем умножаем на 2 канала
			}

			glEnd();

		} // левый канал
		
		tek_pos = (short*)Buffer->data;
		tek_pos++; //правый канал смещён на +2 байта относительно начала буфера

		// правый канал
		{
			glColor3d(0.0, 0.0, 0.7); // цвет графика звука правый канал

			glBegin(GL_LINE_STRIP);

			// для буфера смотрим не вышли ли за его предел
			for (j = 0; j < ::BufferSize / 4; j += StepSize)
			{
				yPos = (double)*tek_pos / 2.0 - 16383.5; // смещаем в нижнюю часть области визуализации
				
				glVertex2d((double)j, yPos);

				tek_pos += 2 * StepSize; // тип short 2 байта, а затем умножаем на 2 канала
			}

			glEnd();

		} // правый канал
	
	} // глубина звука 16 бит

	// глубина звука 24 бит
	if (myBitsPerSample == 24)
	{
		unsigned char* tek_pos = (unsigned char*) Buffer->data;

		// левый канал
		{
			glColor3d(0.0, 0.6, 0.0); // цвет графика звука левый канал

			glBegin(GL_LINE_STRIP);

			// для  буфера смотрим не вышли ли за его предел
			for (j = 0; j < ::BufferSize / 6; j += StepSize)
			{
				yPos = convertSampleToDouble(tek_pos);
				yPos = yPos / 2.0 + 0.5; // смещаем в верхнюю часть области визуализации

				glVertex2d((double)j, yPos);

				tek_pos += 6 * StepSize; // так как в отсчёте 2 канала по 24 бита или 3 байта
			}

			glEnd();

		} // левый канал

		tek_pos = (unsigned char*)Buffer->data;
		tek_pos += 3; //правый канал смещён на +3 байта относительно начала буфера

		// правый канал
		{
			glColor3d(0.0, 0.0, 0.7); // цвет графика звука правый канал

			glBegin(GL_LINE_STRIP);

			// для буфера смотрим не вышли ли за его предел
			for (j = 0; j < ::BufferSize / 6; j += StepSize)
			{
				yPos = convertSampleToDouble(tek_pos);
				yPos = yPos / 2.0 - 0.5; // смещаем в нижнюю часть области визуализации

				glVertex2d((double)j, yPos);

				tek_pos += 6 * StepSize; // так как в отсчёте 2 канала по 24 бита или 3 байта
			}

			glEnd();

		} // правый канал

	} // глубина звука 24 бит

}

void ShowLastBufStereoOverload(rec_list* Buffer) {

	ULONG j;
	double xPos;
	double yPos;// позиция по оси Y при отображении текущей точки

	if (myChannels != 2) return;

	// глубина звука 8 бит
	if (myBitsPerSample == 8) {

		// левый канал
		unsigned char* tek_pos = (unsigned char*)Buffer->data;

		glColor3d(0.0, 0.6, 0.0);

		glBegin(GL_LINE_STRIP);
		// для буфера смотрим не вышли ли за его предел
		for (j = 0; j < ::BufferSize / 2; j += 1) {
			yPos = (double)*tek_pos;
			yPos = yPos - 128.0; // пределы от -128 до 127

			// смещаем в верхнюю часть область визуализации
			glVertex2d(j, yPos / 2.0 + 64.0);

			// проверка на перегрузку
			if (yPos > 126.99 || yPos < -127.99) // (99.99 %)
			{
				LeftOverload99 = OverloadDelay;
			}
			if (yPos > 114 || yPos < -114) // (90 %)
			{
				LeftOverload90 = OverloadDelay;
			}
			if (yPos > 88.5 || yPos < -88.5) // (70 %)
			{
				LeftOverload70 = OverloadDelay;
			}

			tek_pos += 2; // так как в отсчёте 2 канала по 8 бит или 1 байту
		}
		glEnd();

		// правый канал
		tek_pos = (unsigned char*)Buffer->data;
		tek_pos++; //правый канал смещён на +1 байт относительно начала буфера

		glColor3d(0.0, 0.0, 0.7);

		glBegin(GL_LINE_STRIP);
		// для буфера смотрим не вышли ли за его предел
		for (j = 0; j < ::BufferSize / 2; j += 1) {
			yPos = (double)*tek_pos;
			yPos = yPos - 128.0; // пределы от -128 до 127

			// смещаем в нижнюю часть область визуализации
			glVertex2d(j, yPos / 2.0 - 64.0);

			// проверка на перегрузку
			if (yPos > 126.99 || yPos < -127.99) // (99.99 %)
			{
				RightOverload99 = OverloadDelay;
			}
			if (yPos > 114 || yPos < -114) // (90 %)
			{
				RightOverload90 = OverloadDelay;
			}
			if (yPos > 88.5 || yPos < -88.5) // (70 %)
			{
				RightOverload70 = OverloadDelay;
			}

			tek_pos += 2; // так как в отсчёте 2 канала по 8 бит или 1 байту
		}
		glEnd();

	} // if (myBitsPerSample == 8)

	// глубина звука 16 бит
	if (myBitsPerSample == 16) {

		// левый канал
		short* tek_pos = (short*)Buffer->data;

		glColor3d(0.0, 0.6, 0.0);

		glBegin(GL_LINE_STRIP);
		// для  буфера смотрим не вышли ли за его предел
		for (j = 0; j < ::BufferSize / 4; j += 1) {
			yPos = (double)*tek_pos;

			// смещаем в верхнюю часть область визуализации
			glVertex2d(j, yPos / 2 + 16383.5);

			// проверка на перегрузку
			if (yPos > 32763.0 || yPos < -32764.0) // (99.99 %)
			{
				LeftOverload99 = OverloadDelay;
			}
			if (yPos > 29490.0 || yPos < -29490.0) // (90 %)
			{
				LeftOverload90 = OverloadDelay;
			}
			if (yPos > 22936.5 || yPos < -22936.5) // (70 %)
			{
				LeftOverload70 = OverloadDelay;
			}

			tek_pos += 2; // так как в отсчёте 2 канала по 16 бит или 2 байта
		}
		glEnd();

		// правый канал
		tek_pos = (short*)Buffer->data;
		tek_pos++; //правый канал смещён на +2 байта относительно начала буфера

		glColor3d(0.0, 0.0, 0.7);

		glBegin(GL_LINE_STRIP);
		// для буфера смотрим не вышли ли за его предел
		for (j = 0; j < ::BufferSize / 4; j += 1) {
			yPos = (double)*tek_pos;

			// смещаем в нижнюю часть область визуализации
			glVertex2d(j, yPos / 2 - 16383.5);

			// проверка на перегрузку
			if (yPos > 32763.0 || yPos < -32764.0) // (99.99 %)
			{
				RightOverload99 = OverloadDelay;
			}
			if (yPos > 29490.0 || yPos < -29490.0) // (90 %)
			{
				RightOverload90 = OverloadDelay;
			}
			if (yPos > 22936.5 || yPos < -22936.5) // (70 %)
			{
				RightOverload70 = OverloadDelay;
			}

			tek_pos += 2; // так как в отсчёте 2 канала по 16 бит или 2 байта
		}
		glEnd();

	} // if (myBitsPerSample == 16)

	// глубина звука 24 бит
	if (myBitsPerSample == 24) {

		// левый канал
		unsigned char* tek_pos = (unsigned char*)Buffer->data;

		glColor3d(0.0, 0.6, 0.0);

		glBegin(GL_LINE_STRIP);
		// для  буфера смотрим не вышли ли за его предел
		for (j = 0; j < ::BufferSize / 6; j += 1) {
			yPos = convertSampleToDouble(tek_pos);

			// смещаем в верхнюю часть область визуализации
			glVertex2d(j, yPos / 2.0 + 0.5);

			// проверка на перегрузку
			if (yPos > 0.9999 || yPos < -0.9999) // (99.99 %)
			{
				LeftOverload99 = OverloadDelay;
			}
			if (yPos > 0.89999 || yPos < -0.89999) // (90 %)
			{
				LeftOverload90 = OverloadDelay;
			}
			if (yPos > 0.70000 || yPos < -0.70000) // (70 %)
			{
				LeftOverload70 = OverloadDelay;
			}

			tek_pos += 6; // так как в отсчёте 2 канала по 24 бита или 3 байта
		}
		glEnd();

		// правый канал
		tek_pos = (unsigned char*)Buffer->data;
		tek_pos += 3; //правый канал смещён на +3 байта относительно начала буфера

		glColor3d(0.0, 0.0, 0.7);

		glBegin(GL_LINE_STRIP);
		// для буфера смотрим не вышли ли за его предел
		for (j = 0; j < ::BufferSize / 6; j += 1) {
			yPos = convertSampleToDouble(tek_pos);

			// смещаем в нижнюю часть область визуализации
			glVertex2d(j, yPos / 2.0 - 0.5);

			// проверка на перегрузку
			if (yPos > 0.9999 || yPos < -0.9999) // (99.99 %)
			{
				RightOverload99 = OverloadDelay;
			}
			if (yPos > 0.89999 || yPos < -0.89999) // (90 %)
			{
				RightOverload90 = OverloadDelay;
			}
			if (yPos > 0.70000 || yPos < -0.70000) // (70 %)
			{
				RightOverload70 = OverloadDelay;
			}

			tek_pos += 6; // так как в отсчёте 2 канала по 24 бита или 3 байта
		}
		glEnd();

	} // if (myBitsPerSample == 24)


	// отображение значков перегрузки (левый канал)

	yPos = ((double)::glViewportHeight - 15.0) / (double)::glViewportHeight;  // от 0 до 1
	yPos = yPos * (glYmax - glYmin) + glYmin; // фактические координаты по оси Y

	// для 70%

	xPos = 12.0 / (double)::glViewportWidth; // от 0 до 1
	xPos = xPos * glXmax; // фактические координаты по оси X

	// окантовка
	glPointSize(14); // увеличиваем размер точки
	glColor3d(0.1, 0.1, 0.1);
	glBegin(GL_POINTS); // окантовка
	glVertex2d(xPos, yPos);
	glEnd();

	// сама точка отображения перегрузки
	glPointSize(10); // слегка уменьшаем размер точки

	if (LeftOverload70 > 0)
		glColor3d(1, 1, 0.1);
	else
		glColor3d(0.3, 0.3, 0.3);

	glBegin(GL_POINTS); // перегрузка 70%, точка
	glVertex2d(xPos, yPos);
	glEnd();

	// для 90%

	xPos = 25.0 / (double)::glViewportWidth; // от 0 до 1
	xPos = xPos * glXmax; // фактические координаты по оси X

	// окантовка
	glPointSize(14); // увеличиваем размер точки
	glColor3d(0.1, 0.1, 0.1);
	glBegin(GL_POINTS); // окантовка
	glVertex2d(xPos, yPos);
	glEnd();

	// сама точка отображения перегрузки
	glPointSize(10); // слегка уменьшаем размер точки

	if (LeftOverload90 > 0)
		glColor3d(1, 0.7, 0.1);
	else
		glColor3d(0.3, 0.3, 0.3);

	glBegin(GL_POINTS); // перегрузка 90%, точка
	glVertex2d(xPos, yPos);
	glEnd();

	// для 99%

	xPos = 38.0 / (double)::glViewportWidth; // от 0 до 1
	xPos = xPos * glXmax; // фактические координаты по оси X

	// окантовка
	glPointSize(14); // увеличиваем размер точки
	glColor3d(0.1, 0.1, 0.1);
	glBegin(GL_POINTS); // окантовка
	glVertex2d(xPos, yPos);
	glEnd();

	// сама точка отображения перегрузки
	glPointSize(10); // слегка уменьшаем размер точки

	if (LeftOverload99 > 0)
		glColor3d(1, 0.1, 0.1);
	else
		glColor3d(0.3, 0.3, 0.3);

	glBegin(GL_POINTS); // перегрузка 99%, точка
	glVertex2d(xPos, yPos);
	glEnd();


	// отображение значков перегрузки (правый канал)

	yPos = ((double)::glViewportHeight / 2.0 - 15.0) / (double)::glViewportHeight;  // от 0 до 1
	yPos = yPos * (glYmax - glYmin) + glYmin; // фактические координаты по оси Y

	// для 70%

	xPos = 12.0 / (double)::glViewportWidth; // от 0 до 1
	xPos = xPos * glXmax; // фактические координаты по оси X

	// окантовка
	glPointSize(14); // увеличиваем размер точки
	glColor3d(0.1, 0.1, 0.1);
	glBegin(GL_POINTS); // окантовка
	glVertex2d(xPos, yPos);
	glEnd();

	// сама точка отображения перегрузки
	glPointSize(10); // слегка уменьшаем размер точки

	if (RightOverload70 > 0)
		glColor3d(1, 1, 0.1);
	else
		glColor3d(0.3, 0.3, 0.3);

	glBegin(GL_POINTS); // перегрузка 70%, точка
	glVertex2d(xPos, yPos);
	glEnd();

	// для 90%

	xPos = 25.0 / (double)::glViewportWidth; // от 0 до 1
	xPos = xPos * glXmax; // фактические координаты по оси X

	// окантовка
	glPointSize(14); // увеличиваем размер точки
	glColor3d(0.1, 0.1, 0.1);
	glBegin(GL_POINTS); // окантовка
	glVertex2d(xPos, yPos);
	glEnd();

	// сама точка отображения перегрузки
	glPointSize(10); // слегка уменьшаем размер точки

	if (RightOverload90 > 0)
		glColor3d(1, 0.7, 0.1);
	else
		glColor3d(0.3, 0.3, 0.3);

	glBegin(GL_POINTS); // перегрузка 90%, точка
	glVertex2d(xPos, yPos);
	glEnd();

	// для 99%

	xPos = 38.0 / (double)::glViewportWidth; // от 0 до 1
	xPos = xPos * glXmax; // фактические координаты по оси X

	// окантовка
	glPointSize(14); // увеличиваем размер точки
	glColor3d(0.1, 0.1, 0.1);
	glBegin(GL_POINTS); // окантовка
	glVertex2d(xPos, yPos);
	glEnd();

	// сама точка отображения перегрузки
	glPointSize(10); // слегка уменьшаем размер точки

	if (RightOverload99 > 0)
		glColor3d(1, 0.1, 0.1);
	else
		glColor3d(0.3, 0.3, 0.3);

	glBegin(GL_POINTS); // перегрузка 99%, точка
	glVertex2d(xPos, yPos);
	glEnd();


	glPointSize(1); // возвращаем размер точки


	// плавное затухание значков перегрузки
	if (LeftOverload70 > 0)
		LeftOverload70 -= 1;

	if (RightOverload70 > 0)
		RightOverload70 -= 1;

	if (LeftOverload90 > 0)
		LeftOverload90 -= 1;

	if (RightOverload90 > 0)
		RightOverload90 -= 1;

	if (LeftOverload99 > 0)
		LeftOverload99 -= 1;

	if (RightOverload99 > 0)
		RightOverload99 -= 1;

}

void ShowAllBufMono(BOOL localShowPeaks)
{
	// начальные проверки
	if (myChannels != 1)
		return;

	if (Head_rec == NULL)
		return;

	/////////////////////////////////////////////////////////////////////////////
	// делаем мини окошко "Визуализация графика звука..."
	HWND hWndWaitWindow = NULL;
	// Если длина аудио более 10 мин. (для моно, 44КГц, 16 бит)
	if (::RecBufferLenght > 52920000.0)
	{
		// определяем текущий масштаб в системе
		float Scale = (float)WindowDPI / 96.0f; // масштаб окна (в Win XP равен 1.0)

		int widthWaitWindow = int(250 * Scale); // ширина нашего мини окна
		int heightWaitWindow = int(80 * Scale); // высота нашего мини окна

		// получаем размеры и координаты главного окна
		RECT rcParent;
		GetWindowRect(hWnd, &rcParent);

		// мини окно всегда будет в центре главного окна
		int xWaitWindow = rcParent.left + (rcParent.right - rcParent.left - widthWaitWindow) / 2;
		int yWaitWindow = rcParent.top + (rcParent.bottom - rcParent.top - heightWaitWindow) / 2;

		// создаём мини окно
		hWndWaitWindow = CreateWindowEx(
			WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
			L"STATIC",
			L"Визуализация графика звука...",
			WS_POPUP | WS_BORDER | WS_VISIBLE | SS_CENTER,
			xWaitWindow, yWaitWindow, widthWaitWindow, heightWaitWindow,
			hWnd, NULL, hInst, NULL);

		// меняем стиль шрифта мини окна
		SendMessage(hWndWaitWindow, WM_SETFONT, (WPARAM)hFontButton, TRUE);

		UpdateWindow(hWndWaitWindow);

		// ... и блокируем ввод в главное окно (пока идёт обработка данных
		EnableWindow(hWnd, FALSE);

	}
	/////////////////////////////////////////////////////////////////////////////


	int stepSize = 1; // сколько позиций при визуализаци пропускаем (1 = каждая точка)

	double yPos; // позиция по оси Y для текущего отсчёта
	double i, j;


	glColor3d(0.0, 0.55, 0.0);

	// глубина звука 8 бит
	if (myBitsPerSample == 8)
	{
		rec_list* Tek_buf = Head_rec;
		unsigned char* tek_pos;

		// для всего списка подсчитываем каждое X
		glBegin(GL_LINE_STRIP);
		for (i = 0; i < ::glXmax;)
		{
			tek_pos = (unsigned char*)Tek_buf->data;

			// для каждого буфера смотрим не вышли ли за его предел!
			for (j = 0; j < ::BufferSize; j += stepSize)
			{
				yPos = (double)*tek_pos;
				yPos = yPos - 128.0;
				glVertex2d(i + j, yPos);
				tek_pos += stepSize;
			}
			i += ::BufferSize;
			Tek_buf = Tek_buf->next;
		}
		glEnd();

	} // if(myBitsPerSample == 8)

	// глубина звука 16 бит
	if (myBitsPerSample == 16)
	{
		rec_list* Tek_buf = Head_rec;
		short* tek_pos;

		// для всего списка подсчитываем каждое X
		glBegin(GL_LINE_STRIP);
		for (i = 0; i < ::glXmax;)
		{
			tek_pos = (short*)Tek_buf->data;

			// для каждого буфера смотрим не вышли ли за его предел!
			for (j = 0; j < ::BufferSize / 2; j += stepSize)
			{
				glVertex2d(i + j, (double)*tek_pos);
				tek_pos += stepSize;
			}
			i += (::BufferSize / 2);
			Tek_buf = Tek_buf->next;
		}
		glEnd();

	} // if (myBitsPerSample == 16)

	// глубина звука 24 бита
	if (myBitsPerSample == 24)
	{
		rec_list* Tek_buf = Head_rec;
		unsigned char* tek_pos;

		// для всего списка подсчитываем каждое X
		glBegin(GL_LINE_STRIP);
		for (i = 0; i < ::glXmax;)
		{
			tek_pos = (unsigned char*)Tek_buf->data;

			// для каждого буфера смотрим не вышли ли за его предел!
			for (j = 0; j < ::BufferSize / 3; j += stepSize)
			{
				yPos = convertSampleToDouble(tek_pos);
				glVertex2d(i + j, yPos);
				tek_pos = tek_pos + (stepSize * 3);
			}
			i += (::BufferSize / 3);
			Tek_buf = Tek_buf->next;
		}
		glEnd();

	} // if (myBitsPerSample == 24)

	// если нужно, отображаем пики/перегрузку
	if (localShowPeaks == TRUE)
	{
		// мин и макс значения для 8, 16, 24 битного звука
		double minY, maxY;

		// порог срабатывания перегрузки для 8, 16, 24 битного звука (99.99% от мин/макс)
		double minYThreshold, maxYThreshold;

		double xPos; // буферная переменная, позиция по оси Х

		glColor3d(1.0, 0.2, 0.2); // цвет для линий пиков/перегрузки (красный)

		// глубина звука 8 бит
		if (myBitsPerSample == 8)
		{
			minY = -128.0;
			maxY = 127.0;

			minYThreshold = -127.99;
			maxYThreshold = 126.99;

			// для всего звука ищем пики/перегрузку и отображаем
			glBegin(GL_LINES);

			rec_list* Tek_buf = Head_rec;
			unsigned char* tek_pos;

			xPos = 0; // позиция по оси Х

			// для всех хвуковых данных, пока не дошли до конца
			while (Tek_buf != NULL)
			{
				tek_pos = (unsigned char*)Tek_buf->data;

				// для каждого буфера смотрим не вышли ли за его предел
				for (j = 0; j < ::BufferSize; j += 1)
				{
					yPos = (double)*tek_pos;
					yPos = yPos - 128.0;

					if(yPos > maxYThreshold || yPos < minYThreshold)
					{
						glVertex2d(xPos, minY);
						glVertex2d(xPos, maxY);
					}

					tek_pos += 1; // к следующему звуковому отсчёту
					xPos += 1; // позиция по оси X тоже сместилась вправо

				}

				Tek_buf = Tek_buf->next;

			} // для всех хвуковых данных, пока не дошли до конца

			glEnd();

		} // if(myBitsPerSample == 8)

		// глубина звука 16 бит
		if (myBitsPerSample == 16)
		{
			minY = -32768.0;
			maxY = 32767.0;

			minYThreshold = -32764.0;
			maxYThreshold = 32763.0;

			// для всего звука ищем пики/перегрузку и отображаем
			glBegin(GL_LINES);

			rec_list* Tek_buf = Head_rec;
			short* tek_pos;

			xPos = 0; // позиция по оси Х

			// для всех хвуковых данных, пока не дошли до конца
			while (Tek_buf != NULL)
			{
				tek_pos = (short*)Tek_buf->data;

				// для каждого буфера смотрим не вышли ли за его предел
				for (j = 0; j < ::BufferSize; j += 2)
				{
					yPos = (double)*tek_pos;

					if (yPos > maxYThreshold || yPos < minYThreshold)
					{
						glVertex2d(xPos, minY);
						glVertex2d(xPos, maxY);
					}

					tek_pos += 1; // к следующему звуковому отсчёту (tek_pos = 2 байта)
					xPos += 1; // позиция по оси X тоже сместилась вправо

				}

				Tek_buf = Tek_buf->next;

			} // для всех хвуковых данных, пока не дошли до конца

			glEnd();

		} // if(myBitsPerSample == 16)

		// глубина звука 24 бита
		if (myBitsPerSample == 24)
		{
			minY = -1.0;
			maxY = 1.0;

			minYThreshold = -0.9999;
			maxYThreshold = 0.9999;

			// для всего звука ищем пики/перегрузку и отображаем
			glBegin(GL_LINES);

			rec_list* Tek_buf = Head_rec;
			unsigned char* tek_pos;

			xPos = 0; // позиция по оси Х

			// для всех хвуковых данных, пока не дошли до конца
			while (Tek_buf != NULL)
			{
				tek_pos = (unsigned char*)Tek_buf->data;

				// для каждого буфера смотрим не вышли ли за его предел
				for (j = 0; j < ::BufferSize; j += 3)
				{
					yPos = convertSampleToDouble(tek_pos);

					if (yPos > maxYThreshold || yPos < minYThreshold)
					{
						glVertex2d(xPos, minY);
						glVertex2d(xPos, maxY);
					}

					tek_pos += 3; // к следующему звуковому отсчёту (+3 байта)
					xPos += 1; // позиция по оси X тоже сместилась вправо

				}

				Tek_buf = Tek_buf->next;

			} // для всех хвуковых данных, пока не дошли до конца

			glEnd();

		} // if(myBitsPerSample == 24)


	} // if (localShowPeaks == TRUE)

	
	/////////////////////////////////////////////////////////////////////////////
	// если выводили, то убираем мини окошко "Визуализация графика звука..."
	if (hWndWaitWindow != NULL)
	{
		DestroyWindow(hWndWaitWindow);
		// ...снова активируем главное окно
		EnableWindow(hWnd, TRUE);
		// и возвращаем фокус главному окну
		SetForegroundWindow(hWnd);
	}
	/////////////////////////////////////////////////////////////////////////////

}

void ShowLastBufMono(rec_list *Buffer, BOOL ShowAllPoints)
{
	if(myChannels != 1)
		return;

	ULONG StepSize;
	if (ShowAllPoints) 
		StepSize = 1; // визуализируем каждую точку буфера
	else 
		StepSize = 2; // визуализируем через одну точку буфера

	ULONG i;
	double yPos;

	glColor3d(0.0, 0.6, 0.0); // цвет графика МОНО звука

	if (myBitsPerSample == 8) // глубина звука 8 бит
	{
		unsigned char* tek_pos = (unsigned char*)Buffer->data;

		// для всего последнего записанного буфера
		glBegin(GL_LINE_STRIP);

		// для указанного буфера
		for (i = 0; i < ::BufferSize; i += StepSize)
		{
			yPos = (double)*tek_pos;
			yPos = yPos - 128.0;

			glVertex2d((double)i, yPos);

			tek_pos += StepSize; // смещаемся на 8 бит или 1 байт
		}

		glEnd();
	}

	if (myBitsPerSample == 16) // глубина звука 16 бит
	{
		short* tek_pos = (short*)Buffer->data;

		// для всего последнего записанного буфера
		glBegin(GL_LINE_STRIP);

		// для указанного  буфера
		for (i = 0; i < ::BufferSize / 2; i += StepSize)
		{
			glVertex2d((double)i, (double)*tek_pos);

			tek_pos += StepSize; // смещаемся на 16 бит или 2 байта (тип short 2 байта)
		}

		glEnd();
	}

	if (myBitsPerSample == 24) // глубина звука 24 бит
	{
		unsigned char* tek_pos = (unsigned char*)Buffer->data;

		// для всего последнего записанного буфера
		glBegin(GL_LINE_STRIP);

		// для указанного  буфера
		for (i = 0; i < ::BufferSize / 3; i += StepSize)
		{
			yPos = convertSampleToDouble(tek_pos);

			glVertex2d((double)i, yPos);

			tek_pos += 3 * StepSize; // смещаемся на 8 * 3 = 24 бита или 3 байта
		}

		glEnd();
	}

}

void ShowLastBufMonoOverload(rec_list* Buffer) {

	if (myChannels != 1)
		return;

	ULONG i;
	double xPos, yPos;

	glColor3d(0.0, 0.6, 0.0); // цвет графика МОНО канала

	if (myBitsPerSample == 8) { // глубина звука 8 бит

		unsigned char* tek_pos = (unsigned char*)Buffer->data;

		// для всего последнего записанного буфера
		glBegin(GL_LINE_STRIP);
		// для указанного буфера
		for (i = 0; i < ::BufferSize; i++)
		{
			yPos = (double)*tek_pos;
			yPos = yPos - 128.0; // переводим в диапазон -128..+127

			glVertex2d((double)i, yPos);

			// проверка на перегрузку
			if (yPos > 126.99 || yPos < -127.99) // (99.99 %)
			{
				LeftOverload99 = OverloadDelay;
			}
			if (yPos > 114.0 || yPos < -114.0) // (90 %)
			{
				LeftOverload90 = OverloadDelay;
			}
			if (yPos > 88.5 || yPos < -88.5) // (70 %)
			{
				LeftOverload70 = OverloadDelay;
			}

			// смещаемся на 8 бит или 1 байт
			tek_pos++;
		}
		glEnd();

	} // if (myBitsPerSample == 8)

	if (myBitsPerSample == 16) { // глубина звука 16 бит

		short* tek_pos = (short*)Buffer->data;

		// для всего последнего записанного буфера
		glBegin(GL_LINE_STRIP);
		// для указанного  буфера
		for (i = 0; i < ::BufferSize / 2; i++)
		{
			yPos = (double)*tek_pos;
			glVertex2d((double)i, yPos);

			// проверка на перегрузку
			if (yPos > 32763.0 || yPos < -32764.0) // (99.99 %)
			{
				LeftOverload99 = OverloadDelay;
			}
			if (yPos > 29490.0 || yPos < -29490.0) // (90 %)
			{
				LeftOverload90 = OverloadDelay;
			}
			if (yPos > 22936.5 || yPos < -22936.5) // (70 %)
			{
				LeftOverload70 = OverloadDelay;
			}

			tek_pos++; // смещаемся на 16 бит или 2 байта
		}
		glEnd();

	} // if (myBitsPerSample == 16)

	if (myBitsPerSample == 24) { // глубина звука 24 бит

		unsigned char* tek_pos = (unsigned char*)Buffer->data;

		// для всего последнего записанного буфера
		glBegin(GL_LINE_STRIP);
		// для указанного  буфера
		for (i = 0; i < ::BufferSize / 3; i++)
		{
			yPos = convertSampleToDouble(tek_pos);
			glVertex2d((double)i, yPos);

			// проверка на перегрузку
			if (yPos > 0.9999 || yPos < -0.9999) // (99.99 %)
			{
				LeftOverload99 = OverloadDelay;
			}
			if (yPos > 0.89999 || yPos < -0.89999) // (90 %)
			{
				LeftOverload90 = OverloadDelay;
			}
			if (yPos > 0.70000 || yPos < -0.70000) // (70 %)
			{
				LeftOverload70 = OverloadDelay;
			}

			tek_pos += 3; // смещаемся на 24 бита или 3 байта
		}
		glEnd();

	} // if (myBitsPerSample == 24)


	// отображение значков перегрузки

	yPos = ((double)::glViewportHeight - 15.0) / (double)::glViewportHeight;  // от 0 до 1
	yPos = yPos * (glYmax - glYmin) + glYmin; // фактические координаты по оси Y

	// для 70%

	xPos = 12.0 / (double)::glViewportWidth; // от 0 до 1
	xPos = xPos * glXmax; // фактические координаты по оси X

	// окантовка
	glPointSize(14); // увеличиваем размер точки
	glColor3d(0.1, 0.1, 0.1);
	glBegin(GL_POINTS); // окантовка
	glVertex2d(xPos, yPos);
	glEnd();

	// сама точка отображения перегрузки
	glPointSize(10); // слегка уменьшаем размер точки

	if (LeftOverload70 > 0)
		glColor3d(1, 1, 0.1);
	else
		glColor3d(0.3, 0.3, 0.3);

	glBegin(GL_POINTS); // перегрузка 70%, точка
	glVertex2d(xPos, yPos);
	glEnd();

	// для 90%

	xPos = 25.0 / (double)::glViewportWidth; // от 0 до 1
	xPos = xPos * glXmax; // фактические координаты по оси X

	// окантовка
	glPointSize(14); // увеличиваем размер точки
	glColor3d(0.1, 0.1, 0.1);
	glBegin(GL_POINTS); // окантовка
	glVertex2d(xPos, yPos);
	glEnd();

	// сама точка отображения перегрузки
	glPointSize(10); // слегка уменьшаем размер точки

	if (LeftOverload90 > 0)
		glColor3d(1, 0.7, 0.1);
	else
		glColor3d(0.3, 0.3, 0.3);

	glBegin(GL_POINTS); // перегрузка 90%, точка
	glVertex2d(xPos, yPos);
	glEnd();

	// для 99%

	xPos = 38.0 / (double)::glViewportWidth; // от 0 до 1
	xPos = xPos * glXmax; // фактические координаты по оси X

	// окантовка
	glPointSize(14); // увеличиваем размер точки
	glColor3d(0.1, 0.1, 0.1);
	glBegin(GL_POINTS); // окантовка
	glVertex2d(xPos, yPos);
	glEnd();

	// сама точка отображения перегрузки
	glPointSize(10); // слегка уменьшаем размер точки

	if (LeftOverload99 > 0)
		glColor3d(1, 0.1, 0.1);
	else
		glColor3d(0.3, 0.3, 0.3);

	glBegin(GL_POINTS); // перегрузка 99%, точка
	glVertex2d(xPos, yPos);
	glEnd();

	glPointSize(1); // возвращаем размер точки


	// плавное затухание значков перегрузки
	if (LeftOverload70 > 0)
		LeftOverload70 -= 1;

	if (LeftOverload90 > 0)
		LeftOverload90 -= 1;

	if (LeftOverload99 > 0)
		LeftOverload99 -= 1;

}

void ShowPeakMeterMono(rec_list* Buffer)
{
	if (myChannels != 1)
		return;

	static double lastPeaklevel = 0; // храним предыдущее значение Peak Meter (от 0 до 1)

	static double HoldPeak = 0; // храним предыдущее значение Peak‑Hold (от 0 до 1)
	static unsigned short HoldPeakFrames = 0; // сколько ещё кадров удерживаем текущий Peak‑Hold

	static unsigned short uOverload = 0; // храним перегрузку (если больше 0)

	ULONG i;
	double xPos, yPos, xPosEnd;

	double MaxSample = 0; // храним значение максимального отсчёта (по модулю) из нашего звука

	double Peaklevel = 0; // новое значение Peak Meter, нормализованное значение MaxSample (от 0 до 1)

	if (myBitsPerSample == 8) // глубина звука 8 бит
	{
		unsigned char* tek_pos = (unsigned char*)Buffer->data;

		// для указанного буфера ищем отсчёт с максимальным значением по модулю
		for (i = 0; i < ::BufferSize; i++)
		{
			yPos = (double)*tek_pos; // получаем UCHAR, от 0 до 255
			yPos = yPos - 128.0; // переводим в диапазон -128..+127

			// ищем перегрузку
			if (yPos > 126.99 || yPos < -127.99) // (99.99 %)
			{
				uOverload = 40; // пока > 0, отображаем значёк перегрузки
			}

			yPos = fabs(yPos); // находим абсолютное значение

			// ищем отсчёт с максимальным значением
			if (yPos > MaxSample)
				MaxSample = yPos;

			// смещаемся на 8 бит или 1 байт к следующему отсчёту
			tek_pos++;
		}

		// Получаем Peak Meter из MaxSample
		Peaklevel = MaxSample / 128.0;

	} // if (myBitsPerSample == 8)

	if (myBitsPerSample == 16) // глубина звука 16 бит
	{
		short* tek_pos = (short*)Buffer->data;

		// для указанного буфера ищем отсчёт с максимальным значением по модулю
		for (i = 0; i < ::BufferSize / 2; i++)
		{
			yPos = (double)*tek_pos;

			// ищем перегрузку
			if (yPos > 32763.0 || yPos < -32764.0) // (99.99 %)
			{
				uOverload = 40; // пока > 0, отображаем значёк перегрузки
			}

			yPos = fabs(yPos); // находим абсолютное значение

			// ищем отсчёт с максимальным значением
			if (yPos > MaxSample)
				MaxSample = yPos;

			// смещаемся на 16 бит или 2 байта к следующему отсчёту
			tek_pos++;
		}

		// Получаем Peak Meter из MaxSample
		Peaklevel = MaxSample / 32768.0;

	} // if (myBitsPerSample == 16)

	if (myBitsPerSample == 24) // глубина звука 24 бит
	{
		unsigned char* tek_pos = (unsigned char*)Buffer->data;

		// для указанного буфера ищем отсчёт с максимальным значением по модулю
		for (i = 0; i < ::BufferSize / 3; i++)
		{
			yPos = convertSampleToDouble(tek_pos);

			// ищем перегрузку
			if (yPos > 0.9999 || yPos < -0.9999) // (99.99 %)
			{
				uOverload = 40; // пока > 0, отображаем значёк перегрузки
			}

			yPos = fabs(yPos); // находим абсолютное значение

			// ищем отсчёт с максимальным значением
			if (yPos > MaxSample)
				MaxSample = yPos;

			// смещаемся на на 24 бита или 3 байта к следующему отсчёту
			tek_pos += 3;
		}

		// Получаем Peak Meter из MaxSample
		Peaklevel = MaxSample; // ни на что делить не надо, MaxSample уже и так от 0 до 1

	} // if (myBitsPerSample == 24)

	// вычисляем отображаемый уровень Peak Meter
	{
		// если новое значение Peak Meter меньше предыдущего,
		// то делаем затухание плавным
		if (Peaklevel < lastPeaklevel)
			lastPeaklevel = lastPeaklevel - 0.3 * (lastPeaklevel - Peaklevel); // экспоненциальное затухание
		// если новое значение Peak Meter больше предыдущего,
		// то резкий рост разрешен
		else
			lastPeaklevel = Peaklevel;

	} // вычисляем отображаемый уровень Peak Meter

	// Вычисляем новый уровень Peak‑Hold
	{
		// если новый Peak‑Hold больше старого
		if (lastPeaklevel >= HoldPeak)
		{
			HoldPeak = lastPeaklevel;
			HoldPeakFrames = 40; // удерживаем этот пик следующие 40 кадров
		}

		// если новый Peak‑Hold меньше старого
		else
		{
			if (HoldPeakFrames > 0)
			{
				HoldPeakFrames -= 1;
			}

			else
			{
				// разница между Peak‑Hold и lastPeaklevel более 5%
				if (HoldPeak - lastPeaklevel > 0.05)
					HoldPeak -= 0.05; // плавное падение Peak‑Hold

				// разница между Peak‑Hold и lastPeaklevel менее 5%
				else
				{
					HoldPeak = lastPeaklevel;
					HoldPeakFrames = 40;  // удерживаем этот пик следующие 40 кадров
				}
			}

		} // если новый Peak‑Hold меньше старого

	} // Вычисляем новый уровень Peak‑Hold


	// отображаем уровень Peak Meter
	{

		yPos = ((double)::glViewportHeight - 15.0) / (double)::glViewportHeight;  // от 0 до 1
		yPos = yPos * (glYmax - glYmin) + glYmin;

		// фон Peak Meter
		{
			// ширина окантовки Peak Meter  = 200 + 4 пикселей (2 слева и 2 справа)

			// начальная точка
			xPos = 48.0 / (double)::glViewportWidth; // от 0 до 1
			xPos = xPos * glXmax; // фактические координаты по оси X

			// конечная точка
			xPosEnd = 252.0 / (double)::glViewportWidth; // от 0 до 1
			xPosEnd = xPosEnd * glXmax; // фактические координаты по оси X

			glColor3d(0.0, 0.0, 0.0); // цвет

			DrawHorizontalLine(xPos, xPosEnd, yPos, 14.0);

			// начальная точка
			xPos = 50.0 / (double)::glViewportWidth; // от 0 до 1
			xPos = xPos * glXmax; // фактические координаты по оси X

			// конечная точка
			xPosEnd = 250.0 / (double)::glViewportWidth; // от 0 до 1
			xPosEnd = xPosEnd * glXmax; // фактические координаты по оси X

			glColor3d(0.4, 0.4, 0.4); // цвет

			DrawHorizontalLine(xPos, xPosEnd, yPos, 10.0);

		} // фон Peak Meter

		// Peak Meter
		{
			// начальная точка
			xPos = 50.0 / (double)::glViewportWidth; // от 0 до 1
			xPos = xPos * glXmax; // фактические координаты по оси X

			// конечная точка
			// максимальная ширина Peak Meter 200 пикселей
			xPosEnd = (50.0 + (lastPeaklevel * 200.0)) / (double)::glViewportWidth; // от 0 до 1
			xPosEnd = xPosEnd * glXmax; // фактические координаты по оси X

			glColor3d(0.3, 1.0, 0.3); // цвет

			DrawHorizontalLine(xPos, xPosEnd, yPos, 10.0);

		} // Peak Meter

	} // отображаем уровень Peak Meter


	// Включаем эффект прозрачности (blending)
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// отображаем уровень Peak‑Hold
	{
		// yPos остаётся такое же как у Peak Meter

		double VisualHoldPeak = HoldPeak;

		// если HoldPeak менее 5 %
		if (VisualHoldPeak < 0.05) {
			VisualHoldPeak = 0.05;
			glColor4d(1.0, 1.0, 1.0, 0.5); // полупрозрачный цвет
		}
		else
			glColor4d(1.0, 1.0, 1.0, 0.99); // сплошной цвет


		// начальная точка
		xPos = (50.0 + (VisualHoldPeak * 200.0)) / (double)::glViewportWidth; // от 0 до 1
		xPos = xPos * glXmax; // фактические координаты по оси X

		// конечная точка
		// влево на 4 пикселя
		xPosEnd = (46.0 + (VisualHoldPeak * 200.0)) / (double)::glViewportWidth; // от 0 до 1
		xPosEnd = xPosEnd * glXmax; // фактические координаты по оси X

		DrawHorizontalLine(xPos, xPosEnd, yPos, 10.0);

	} // отображаем уровень Peak‑Hold

	// отображение шкалы поверх Peak Meter (от 0 до 100%)
	{
		// yPos остаётся такое же как у Peak Meter

		glColor4d(0.0, 0.0, 0.0, 0.5); // чёрный цвет + полупрозрачность

		for (i = 5; i < 100; i += 5)
		{
			xPos = (50.0 + (i * 2)) / (double)::glViewportWidth; // от 0 до 1
			xPos = xPos * glXmax; // фактические координаты по оси X

			if (i == 50)
				DrawQuadPoint(xPos, yPos, 4.0);
			else if (i % 10 == 0)
				DrawQuadPoint(xPos, yPos, 2.0);
			else
				DrawQuadPoint(xPos, yPos, 1.0);

		}

	} // отображение шкалы поверх Peak Meter (от 0 до 100%)

	// Возвращаем прозрачность (blending) в исходное состояние
	glDisable(GL_BLEND);


	// отображение значка Перегрузки
	{
		// yPos остаётся такое же как у Peak Meter

		xPos = 265.0 / (double)::glViewportWidth; // от 0 до 1
		xPos = xPos * glXmax; // фактические координаты по оси X

		// окантовка индикатора перегрузки
		{
			glColor3d(0.0, 0.0, 0.0); // цвет

			DrawQuadPoint(xPos, yPos, 14);

		} // окантовка индикатора перегрузки

		// индикатор перегрузки		
		{
			if (uOverload > 0) // если перегрузка есть
			{

				glColor3d((double)uOverload / 40.0 * 0.1 + 0.9, 0.4, 0.2); // цвет

				DrawQuadPoint(xPos, yPos, 10);
				
			}
			else // если перегрузки нет
			{
				glColor3d(0.4, 0.2, 0.1); // цвет
				
				DrawQuadPoint(xPos, yPos, 10);
			}

			// первые 5 отсчётов (0.25 сек) отображаем белую точку в центре
			if (uOverload > 35)
			{
				glColor3d(1.0, 1.0, 1.0); // цвет

				DrawQuadPoint(xPos, yPos, 4);
			}

		} // индикатор перегрузки	

	} // отображение значка Перегрузки

	// задержка отображения перегрузки, чтобы её не пропустить
	// 40 следующих кадров, 2.0 секунды
	if (uOverload > 0)
	{
		uOverload -= 1;
	}

/* // Текст внутри области OpenGL

glColor3f(0.0f, 0.5f, 0.0f);
glMatrixMode(GL_PROJECTION);
glPushMatrix();
glLoadIdentity();

int vp[4];
glGetIntegerv(GL_VIEWPORT, vp);
gluOrtho2D(0, vp[2], 0, vp[3]);

glMatrixMode(GL_MODELVIEW);
glPushMatrix();
glLoadIdentity();

glRasterPos2f(10, 10);

SelectObject(hDC, hFonthList1);
GLuint fontBase = glGenLists(256);
wglUseFontBitmapsA(hDC, 0, 255, fontBase);

// DeleteObject(hFont);


glListBase(fontBase);
glCallLists(strlen("Hello World!"), GL_UNSIGNED_BYTE, "Hello World!");

glPopMatrix();
glMatrixMode(GL_PROJECTION);
glPopMatrix();
glMatrixMode(GL_MODELVIEW);


//*/

	// glLineWidth(1.0f); // возвращаем толщину линии

	glPointSize(1.0f); // возвращаем размер точки

}

void ShowPeakMeterStereo(rec_list* Buffer) 
{
	if (myChannels != 2) return;

	static double lastPeaklevelL = 0; // предыдущее значение Peak Meter (от 0 до 1) левый канал
	static double lastPeaklevelR = 0; // предыдущее значение Peak Meter (от 0 до 1) правый канал

	static double HoldPeakL = 0; // предыдущее значение Peak‑Hold (от 0 до 1) левый канал
	static double HoldPeakR = 0; // предыдущее значение Peak‑Hold (от 0 до 1) правый канал

	static unsigned short HoldPeakFramesL = 0; // сколько ещё кадров удерживаем текущий Peak‑Hold, левый
	static unsigned short HoldPeakFramesR = 0; // сколько ещё кадров удерживаем текущий Peak‑Hold, правый

	static unsigned short uOverloadL = 0; // храним перегрузку (если больше 0), левый канал
	static unsigned short uOverloadR = 0; // храним перегрузку (если больше 0), правый канал

	double MaxSampleL = 0; // значение максимального отсчёта (по модулю) из нашего звука, левый канал
	double MaxSampleR = 0; // значение максимального отсчёта (по модулю) из нашего звука, правый канал

	double PeaklevelL = 0; // новое значение Peak Meter (от 0 до 1), левый канал
	double PeaklevelR = 0; // новое значение Peak Meter (от 0 до 1), правый канал

	ULONG i;
	double xPos, xPosEnd, yPosL, yPosR;

	if (myBitsPerSample == 8) // глубина звука 8 бит
	{
		unsigned char* tek_pos = (unsigned char*)Buffer->data;

		// для указанного буфера ищем отсчёт с максимальным значением по модулю
		for (i = 0; i < ::BufferSize / 2; i++)
		{
			// левый канал
			{
				yPosL = (double)*tek_pos; // получаем UCHAR, от 0 до 255
				yPosL = yPosL - 128.0; // переводим в диапазон -128..+127

				// ищем перегрузку
				if (yPosL > 126.99 || yPosL < -127.99) // (99.99 %)
				{
					uOverloadL = 40; // пока > 0, отображаем значёк перегрузки
				}

				yPosL = fabs(yPosL); // находим абсолютное значение

				// ищем отсчёт с максимальным значением
				if (yPosL > MaxSampleL)
					MaxSampleL = yPosL;

			} // левый канал

			// смещаемся на 8 бит или 1 байт к правому каналу
			tek_pos++;

			// правый канал
			{
				yPosR = (double)*tek_pos; // получаем UCHAR, от 0 до 255
				yPosR = yPosR - 128.0; // переводим в диапазон -128..+127

				// ищем перегрузку
				if (yPosR > 126.99 || yPosR < -127.99) // (99.99 %)
				{
					uOverloadR = 40; // пока > 0, отображаем значёк перегрузки
				}

				yPosR = fabs(yPosR); // находим абсолютное значение

				// ищем отсчёт с максимальным значением
				if (yPosR > MaxSampleR)
					MaxSampleR = yPosR;

			} // правый канал

			// смещаемся на 8 бит или 1 байт к следующему отсчёту
			tek_pos++;

		} // для указанного буфера ищем отсчёт с максимальным значением по модулю

		// Получаем новое значение Peak Meter из MaxSample
		PeaklevelL = MaxSampleL / 128.0; // левый канал
		PeaklevelR = MaxSampleR / 128.0; // правый канал

	} // if (myBitsPerSample == 8)

	if (myBitsPerSample == 16) // глубина звука 16 бит
	{
		short* tek_pos = (short*)Buffer->data;

		// для указанного буфера ищем отсчёт с максимальным значением по модулю
		for (i = 0; i < ::BufferSize / 4; i++)
		{
			// левый канал
			{
				yPosL = (double)*tek_pos;

				// ищем перегрузку
				if (yPosL > 32763.0 || yPosL < -32764.0) // (99.99 %)
				{
					uOverloadL = 40; // пока > 0, отображаем значёк перегрузки
				}

				yPosL = fabs(yPosL); // находим абсолютное значение

				// ищем отсчёт с максимальным значением
				if (yPosL > MaxSampleL)
					MaxSampleL = yPosL;

			} // левый канал

			// смещаемся на 16 бит или 2 байта к правому каналу
			tek_pos++;

			// правый канал
			{
				yPosR = (double)*tek_pos;

				// ищем перегрузку
				if (yPosR > 32763.0 || yPosR < -32764.0) // (99.99 %)
				{
					uOverloadR = 40; // пока > 0, отображаем значёк перегрузки
				}

				yPosR = fabs(yPosR); // находим абсолютное значение

				// ищем отсчёт с максимальным значением
				if (yPosR > MaxSampleR)
					MaxSampleR = yPosR;
			
			} // правый канал

			// смещаемся на 16 бит или 2 байта к следующему отсчёту
			tek_pos++;
		
		} // для указанного буфера ищем отсчёт с максимальным значением по модулю

		// Получаем новое значение Peak Meter из MaxSample
		PeaklevelL = MaxSampleL / 32768.0; // левый канал
		PeaklevelR = MaxSampleR / 32768.0; // правый канал

	} // if (myBitsPerSample == 16)

	if (myBitsPerSample == 24) // глубина звука 24 бит
	{
		unsigned char* tek_pos = (unsigned char*)Buffer->data;

		// для указанного буфера ищем отсчёт с максимальным значением по модулю
		for (i = 0; i < ::BufferSize / 6; i++)
		{
			// левый канал
			{
				yPosL = convertSampleToDouble(tek_pos);

				// ищем перегрузку
				if (yPosL > 0.9999 || yPosL < -0.9999) // (99.99 %)
				{
					uOverloadL = 40; // пока > 0, отображаем значёк перегрузки
				}

				yPosL = fabs(yPosL); // находим абсолютное значение

				// ищем отсчёт с максимальным значением
				if (yPosL > MaxSampleL)
					MaxSampleL = yPosL;
			
			} // левый канал

			// смещаемся на на 24 бита или 3 байта к правому каналу
			tek_pos += 3;

			// правый канал
			{
				yPosR = convertSampleToDouble(tek_pos);

				// ищем перегрузку
				if (yPosR > 0.9999 || yPosR < -0.9999) // (99.99 %)
				{
					uOverloadR = 40; // пока > 0, отображаем значёк перегрузки
				}

				yPosR = fabs(yPosR); // находим абсолютное значение

				// ищем отсчёт с максимальным значением
				if (yPosR > MaxSampleR)
					MaxSampleR = yPosR;

			} // правый канал

			// смещаемся на на 24 бита или 3 байта к следующему отсчёту
			tek_pos += 3;
		
		} // для указанного буфера ищем отсчёт с максимальным значением по модулю

		// Получаем новое значение Peak Meter из MaxSample
		// ни на что делить не надо, MaxSample уже и так от 0 до 1
		PeaklevelL = MaxSampleL; // левый канал
		PeaklevelR = MaxSampleR; // правый канал

	} // if (myBitsPerSample == 24)

	// вычисляем отображаемый уровень Peak Meter
	{
		// левый канал
		{
			// если новое значение Peak Meter меньше предыдущего,
			// то делаем затухание плавным
			if (PeaklevelL < lastPeaklevelL)
				lastPeaklevelL = lastPeaklevelL - 0.3 * (lastPeaklevelL - PeaklevelL); // экспоненциальное затухание
			// если новое значение Peak Meter больше предыдущего,
			// то резкий рост разрешен
			else
				lastPeaklevelL = PeaklevelL;
		}

		// правый канал
		{
			// если новое значение Peak Meter меньше предыдущего,
			// то делаем затухание плавным
			if (PeaklevelR < lastPeaklevelR)
				lastPeaklevelR = lastPeaklevelR - 0.3 * (lastPeaklevelR - PeaklevelR); // экспоненциальное затухание
			// если новое значение Peak Meter больше предыдущего,
			// то резкий рост разрешен
			else
				lastPeaklevelR = PeaklevelR;
		}

	} // вычисляем отображаемый уровень Peak Meter

	// Вычисляем новый уровень Peak‑Hold
	{
		// левый канал
		{
			// если новый Peak‑Hold больше старого
			if (lastPeaklevelL >= HoldPeakL)
			{
				HoldPeakL = lastPeaklevelL;
				HoldPeakFramesL = 40; // удерживаем этот пик следующие 40 кадров
			}

			// если новый Peak‑Hold меньше старого
			else
			{
				if (HoldPeakFramesL > 0)
				{
					HoldPeakFramesL -= 1;
				}

				else
				{
					// разница между Peak‑Hold и lastPeaklevel более 5%
					if (HoldPeakL - lastPeaklevelL > 0.05)
						HoldPeakL -= 0.05; // плавное падение Peak‑Hold

					// разница между Peak‑Hold и lastPeaklevel менее 5%
					else
					{
						HoldPeakL = lastPeaklevelL;
						HoldPeakFramesL = 40;  // удерживаем этот пик следующие 40 кадров
					}
				}

			} // если новый Peak‑Hold меньше старого

		} // левый канал

		// правый канал
		{
			// если новый Peak‑Hold больше старого
			if (lastPeaklevelR >= HoldPeakR)
			{
				HoldPeakR = lastPeaklevelR;
				HoldPeakFramesR = 40; // удерживаем этот пик следующие 40 кадров
			}

			// если новый Peak‑Hold меньше старого
			else
			{
				if (HoldPeakFramesR > 0)
				{
					HoldPeakFramesR -= 1;
				}

				else
				{
					// разница между Peak‑Hold и lastPeaklevel более 5%
					if (HoldPeakR - lastPeaklevelR > 0.05)
						HoldPeakR -= 0.05; // плавное падение Peak‑Hold

					// разница между Peak‑Hold и lastPeaklevel менее 5%
					else
					{
						HoldPeakR = lastPeaklevelR;
						HoldPeakFramesR = 40;  // удерживаем этот пик следующие 40 кадров
					}
				}

			} // если новый Peak‑Hold меньше старого

		} // правый канал

	} // Вычисляем новый уровень Peak‑Hold

	// отображаем уровень Peak Meter
	{
		// левый канал
		yPosL = ((double)::glViewportHeight - 15.0) / (double)::glViewportHeight;  // от 0 до 1
		yPosL = yPosL * (glYmax - glYmin) + glYmin;

		// правый канал
		yPosR = ((double)::glViewportHeight / 2 - 15.0) / (double)::glViewportHeight;  // от 0 до 1
		yPosR = yPosR * (glYmax - glYmin) + glYmin;

		// фон Peak Meter
		{
			// ширина окантовки Peak Meter  = 200 + 4 пикселей (2 слева и 2 справа)

			// начальная точка
			xPos = 48.0 / (double)::glViewportWidth; // от 0 до 1
			xPos = xPos * glXmax; // фактические координаты по оси X

			// конечная точка
			xPosEnd = 252.0 / (double)::glViewportWidth; // от 0 до 1
			xPosEnd = xPosEnd * glXmax; // фактические координаты по оси X

			glColor3d(0.0, 0.0, 0.0); // цвет

			// левый канал
			DrawHorizontalLine(xPos, xPosEnd, yPosL, 14.0);

			// правый канал
			DrawHorizontalLine(xPos, xPosEnd, yPosR, 14.0);

			// начальная точка
			xPos = 50.0 / (double)::glViewportWidth; // от 0 до 1
			xPos = xPos * glXmax; // фактические координаты по оси X

			// конечная точка
			xPosEnd = 250.0 / (double)::glViewportWidth; // от 0 до 1
			xPosEnd = xPosEnd * glXmax; // фактические координаты по оси X

			glColor3d(0.4, 0.4, 0.4); // цвет

			// левый канал
			DrawHorizontalLine(xPos, xPosEnd, yPosL, 10.0);

			// правый канал
			DrawHorizontalLine(xPos, xPosEnd, yPosR, 10.0);

		} // фон Peak Meter

		// Peak Meter
		{
			glColor3d(0.3, 1.0, 0.3); // цвет

			// начальная точка (левый и правый канал)
			xPos = 50.0 / (double)::glViewportWidth; // от 0 до 1
			xPos = xPos * glXmax; // фактические координаты по оси X

			// левый канал
			{
				// yPosL уже вычислили

				// конечная точка
				// максимальная ширина Peak Meter 200 пикселей
				xPosEnd = (50.0 + (lastPeaklevelL * 200.0)) / (double)::glViewportWidth; // от 0 до 1
				xPosEnd = xPosEnd * glXmax; // фактические координаты по оси X

				DrawHorizontalLine(xPos, xPosEnd, yPosL, 10.0);
			
			} // левый канал

			// правый канал
			{
				// yPosR уже вычислили

				// конечная точка
				// максимальная ширина Peak Meter 200 пикселей
				xPosEnd = (50.0 + (lastPeaklevelR * 200.0)) / (double)::glViewportWidth; // от 0 до 1
				xPosEnd = xPosEnd * glXmax; // фактические координаты по оси X

				DrawHorizontalLine(xPos, xPosEnd, yPosR, 10.0);
			
			} // правый канал

		} // Peak Meter

	} // отображаем уровень Peak Meter


	// Включаем эффект прозрачности (blending)
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// отображаем уровень Peak‑Hold
	{
		double VisualHoldPeak;

		// левый канал
		{
			// yPosL остаётся такое же как у Peak Meter

			VisualHoldPeak = HoldPeakL;

			// если HoldPeak менее 5 %
			if (VisualHoldPeak < 0.05) {
				VisualHoldPeak = 0.05;
				glColor4d(1.0, 1.0, 1.0, 0.5); // полупрозрачный цвет
			}
			else
				glColor4d(1.0, 1.0, 1.0, 0.99); // сплошной цвет

			// начальная точка
			xPos = (50.0 + (VisualHoldPeak * 200.0)) / (double)::glViewportWidth; // от 0 до 1
			xPos = xPos * glXmax; // фактические координаты по оси X

			// конечная точка
			// влево на 4 пикселя
			xPosEnd = (46.0 + (VisualHoldPeak * 200.0)) / (double)::glViewportWidth; // от 0 до 1
			xPosEnd = xPosEnd * glXmax; // фактические координаты по оси X

			DrawHorizontalLine(xPos, xPosEnd, yPosL, 10.0);

		} // левый канал

		// правый канал
		{
			// yPosR остаётся такое же как у Peak Meter

			VisualHoldPeak = HoldPeakR;

			// если HoldPeak менее 5 %
			if (VisualHoldPeak < 0.05) {
				VisualHoldPeak = 0.05;
				glColor4d(1.0, 1.0, 1.0, 0.5); // полупрозрачный цвет
			}
			else
				glColor4d(1.0, 1.0, 1.0, 0.99); // сплошной цвет

			// начальная точка
			xPos = (50.0 + (VisualHoldPeak * 200.0)) / (double)::glViewportWidth; // от 0 до 1
			xPos = xPos * glXmax; // фактические координаты по оси X

			// конечная точка
			// влево на 4 пикселя
			xPosEnd = (46.0 + (VisualHoldPeak * 200.0)) / (double)::glViewportWidth; // от 0 до 1
			xPosEnd = xPosEnd * glXmax; // фактические координаты по оси X

			DrawHorizontalLine(xPos, xPosEnd, yPosR, 10.0);

		} // правый канал

	} // отображаем уровень Peak‑Hold

	// отображение шкалы поверх Peak Meter (от 0 до 100%)
	{
		// yPosL остаётся такое же как у Peak Meter

		// yPosR остаётся такое же как у Peak Meter

		glColor4d(0.0, 0.0, 0.0, 0.5); // черный цвет + полупрозрачный

		for (i = 5; i < 100; i += 5)
		{
			xPos = (50.0 + (i * 2)) / (double)::glViewportWidth; // от 0 до 1
			xPos = xPos * glXmax; // фактические координаты по оси X

			if (i == 50)
			{
				// левый канал
				DrawQuadPoint(xPos, yPosL, 4.0);
				// правый канал
				DrawQuadPoint(xPos, yPosR, 4.0);
			}
			else if (i % 10 == 0)
			{
				// левый канал
				DrawQuadPoint(xPos, yPosL, 2.0);
				// правый канал
				DrawQuadPoint(xPos, yPosR, 2.0);
			}
			else
			{
				// левый канал
				DrawQuadPoint(xPos, yPosL, 1.0);
				// правый канал
				DrawQuadPoint(xPos, yPosR, 1.0);
			}

		} // for (i = 5; i < 100; i += 5)

	} // отображение шкалы поверх Peak Meter (от 0 до 100%)

	// Возвращаем прозрачность (blending) в исходное состояние
	glDisable(GL_BLEND);


	// отображение значков Перегрузки
	{
		// yPosL остаётся такое же как у Peak Meter

		// yPosR остаётся такое же как у Peak Meter

		xPos = 265.0 / (double)::glViewportWidth; // от 0 до 1
		xPos = xPos * glXmax; // фактические координаты по оси X

		// окантовка индикатора перегрузки
		{
			glColor3d(0.0, 0.0, 0.0); // цвет

			// левый канал
			DrawQuadPoint(xPos, yPosL, 14);

			// правый канал
			DrawQuadPoint(xPos, yPosR, 14);

		} // окантовка индикатора перегрузки

		// индикатор перегрузки		
		{
			// левый канал
			{

				if (uOverloadL > 0) // если перегрузка есть
				{
					glColor3d((double)uOverloadL / 40.0 * 0.1 + 0.9, 0.4, 0.2); // цвет

					DrawQuadPoint(xPos, yPosL, 10);
				}
				else // если перегрузки нет
				{
					glColor3d(0.4, 0.2, 0.1); // цвет

					DrawQuadPoint(xPos, yPosL, 10);
				}

				// первые 5 отсчётов (0.25 сек) отображаем белую точку в центре
				if (uOverloadL > 35)
				{
					glColor3d(1.0, 1.0, 1.0); // цвет

					DrawQuadPoint(xPos, yPosL, 4);
				}
			
			} // левый канал

			// правый канал
			{

				if (uOverloadR > 0) // если перегрузка есть
				{
					glColor3d((double)uOverloadR / 40.0 * 0.1 + 0.9, 0.4, 0.2); // цвет

					DrawQuadPoint(xPos, yPosR, 10);
				}
				else // если перегрузки нет
				{
					glColor3d(0.4, 0.2, 0.1); // цвет

					DrawQuadPoint(xPos, yPosR, 10);
				}

				// первые 5 отсчётов (0.25 сек) отображаем белую точку в центре
				if (uOverloadR > 35)
				{
					glColor3d(1.0, 1.0, 1.0); // цвет

					DrawQuadPoint(xPos, yPosR, 4);
				}
			
			} // правый канал

		} // индикатор перегрузки	

	} // отображение значков Перегрузки

	// задержка отображения срабатывания перегрузки, чтобы её не пропустить
	// 40 следующих кадров = 2 сек
	if (uOverloadL > 0) // левый канал
	{
		uOverloadL -= 1;
	}
	if (uOverloadR > 0) // правый канал
	{
		uOverloadR -= 1;
	}


	// glLineWidth(1.0f); // возвращаем толщину линии

	glPointSize(1.0f); // возвращаем размер точки

}

void ShowVUMeterMono(rec_list* Buffer)
{
	if (myChannels != 1)
		return;

	static double lastVUlevel = 0.0; // храним предыдущее значение VU Meter (от 0 до 1)

	ULONG i;
	double xPos, yPos, xPosEnd;

	double RMS = 0.0; // значение RMS
	double normalizedRMS = 0.0; // значение нормализованного RMS (от 0 до 1)

	if (myBitsPerSample == 8) // глубина звука 8 бит
	{
		unsigned char* tek_pos = (unsigned char*)Buffer->data;

		// для указанного буфера вычисляем RMS
		for (i = 0; i < ::BufferSize; i++)
		{
			yPos = (double)*tek_pos;
			yPos = yPos - 128.0; // переводим в диапазон -128..+127

			RMS += yPos * yPos;

			// смещаемся на 8 бит или 1 байт к следующему отсчёту
			tek_pos++;
		}

		RMS = sqrt(RMS / ::BufferSize);

		// нормализация RMS (приводим в диапазон от 0 до 1)
		// максимум RMS для 8 бит = 127

		normalizedRMS = RMS / 127.0; // * 2.0 - было раньше;

		if (normalizedRMS > 1.0)
			normalizedRMS = 1.0;

	} // if (myBitsPerSample == 8)

	if (myBitsPerSample == 16) // глубина звука 16 бит
	{
		short* tek_pos = (short*)Buffer->data;

		// для указанного буфера вычисляем RMS
		for (i = 0; i < ::BufferSize; i += 2)
		{
			yPos = (double)*tek_pos;

			RMS += yPos * yPos;

			// смещаемся на 16 бит или 2 байта к следующему отсчёту
			tek_pos++; // short 2 байта
		}

		RMS = sqrt(RMS / ::BufferSize * 2); // кол-во записей равно (BufferSize / 2)

		// нормализация RMS (приводим в диапазон от 0 до 1)
		// максимум RMS для 16 бит = 32767

		normalizedRMS = RMS / 32767.0; // * 2.0 - было раньше;

		if (normalizedRMS > 1.0)
			normalizedRMS = 1.0;

	} // if (myBitsPerSample == 16)

	if (myBitsPerSample == 24) // глубина звука 24 бит
	{
		unsigned char* tek_pos = (unsigned char*)Buffer->data;

		// для указанного буфера вычисляем RMS
		for (i = 0; i < ::BufferSize; i += 3)
		{
			yPos = convertSampleToDouble(tek_pos);

			RMS += yPos * yPos;

			// смещаемся на на 24 бита или 3 байта к следующему отсчёту
			tek_pos += 3;
		}

		RMS = sqrt(RMS / ::BufferSize * 3); // кол-во записей равно (BufferSize / 3)

		// нормализация RMS (приводим в диапазон от 0 до 1)
		// максимум RMS для 24 бит = +1.0

		normalizedRMS = RMS / 1.0; // * 2.0 - было раньше;

		if (normalizedRMS > 1.0)
			normalizedRMS = 1.0;

	} // if (myBitsPerSample == 24)


	// вычисляем отображаемый уровень VU Meter
	{
		// если новое значение VU Meter меньше предыдущего,
		// то делаем затухание плавным
		if (normalizedRMS < lastVUlevel)
			lastVUlevel = lastVUlevel - 0.3 * (lastVUlevel - normalizedRMS); // экспоненциальное затухание
		// если новое значение VU Meter больше предыдущего,
		// то резкий рост разрешен
		else
			lastVUlevel = normalizedRMS;

	} // вычисляем отображаемый уровень VU Meter


	// отображаем уровень VU Meter
	{
		yPos = ((double)::glViewportHeight - 15.0) / (double)::glViewportHeight;  // от 0 до 1
		yPos = yPos * (glYmax - glYmin) + glYmin;

		// фон VU Meter
		{
			// ширина окантовки VU Meter  = 200 + 4 пикселей (2 слева и 2 справа)

			// начальная точка
			xPos = ((double)::glViewportWidth - 212.0) / (double)::glViewportWidth; // от 0 до 1
			xPos = xPos * glXmax; // фактические координаты по оси X

			// конечная точка
			xPosEnd = ((double)::glViewportWidth - 8.0) / (double)::glViewportWidth; // от 0 до 1
			xPosEnd = xPosEnd * glXmax; // фактические координаты по оси X

			glColor3d(0.0, 0.0, 0.0); // цвет

			DrawHorizontalLine(xPos, xPosEnd, yPos, 14.0);

			// начальная точка
			xPos = ((double)::glViewportWidth - 210.0) / (double)::glViewportWidth; // от 0 до 1
			xPos = xPos * glXmax; // фактические координаты по оси X

			// конечная точка
			xPosEnd = ((double)::glViewportWidth - 10.0) / (double)::glViewportWidth; // от 0 до 1
			xPosEnd = xPosEnd * glXmax; // фактические координаты по оси X

			glColor3d(0.4, 0.4, 0.4); // цвет

			DrawHorizontalLine(xPos, xPosEnd, yPos, 10.0);

		} // фон VU Meter

		// VU Meter
		{
			// начальная точка
			xPos = ((double)::glViewportWidth - 210.0) / (double)::glViewportWidth; // от 0 до 1
			xPos = xPos * glXmax; // фактические координаты по оси X

			// конечная точка
			// максимальная ширина VU Meter 200 пикселей
			xPosEnd = ((lastVUlevel * 200.0) + (double)::glViewportWidth - 210.0) / (double)::glViewportWidth; // от 0 до 1
			xPosEnd = xPosEnd * glXmax; // фактические координаты по оси X

			glColor3d(0.0, 0.9, 0.0); // цвет

			DrawHorizontalLine(xPos, xPosEnd, yPos, 10.0);

		} // VU Meter

	} // отображаем уровень VU Meter

	
	// включаем эффект прозрачности (blending)
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// отображение шкалы поверх VU Meter (от 0 до 100%)
	{
		// yPos такое же как и у уровня VU Meter

		glColor4d(0.0, 0.0, 0.0, 0.5); // чёрный цвет и полупрозрачный

		for (i = 5; i < 100; i += 5)
		{
			xPos = ((i * 2) + (double)::glViewportWidth - 210.0) / (double)::glViewportWidth; // от 0 до 1
			xPos = xPos * glXmax; // фактические координаты по оси X

			if (i == 50)
			{
				DrawQuadPoint(xPos, yPos, 4.0);
			}
			else if (i % 10 == 0)
			{
				DrawQuadPoint(xPos, yPos, 2.0);
			}
			else
			{
				DrawQuadPoint(xPos, yPos, 1.0);
			}

		}

	} // отображение шкалы поверх VU Meter (от 0 до 100%)

	// Возвращаем прозрачность (blending) в исходное состояние
	glDisable(GL_BLEND);


	// glLineWidth(1.0f); // возвращаем толщину линии

	glPointSize(1.0f); // возвращаем размер точки

}

void ShowVUMeterStereo(rec_list* Buffer)
{
	if (myChannels != 2)
		return;

	// храним предыдущие значение VU Meter для левого и правого канала (от 0 до 1)
	static double lastVUlevelL = 0.0;
	static double lastVUlevelR = 0.0;

	// храним новые значение VU Meter для левого и правого канала (от 0 до 1)
	double VUlevelL = 0.0;
	double VUlevelR = 0.0;

	ULONG i;
	double xPos, xPosEnd, yPosL, yPosR;

	if (myBitsPerSample == 8) // глубина звука 8 бит
	{

		unsigned char* tek_pos = (unsigned char*)Buffer->data;

		// для указанного буфера вычисляем RMS
		for (i = 0; i < ::BufferSize; i += 2)
		{
			// левый канал
			yPosL = (double)tek_pos[0];
			yPosL = yPosL - 128.0; // переводим в диапазон -128..+127

			VUlevelL += yPosL * yPosL;

			// правый канал
			yPosR = (double)tek_pos[1];
			yPosR = yPosR - 128.0; // переводим в диапазон -128..+127

			VUlevelR += yPosR * yPosR;

			// смещаемся на 8*2 бит или 2 байта, к следующему отсчёту
			tek_pos += 2;
		}

		// левый канал
		VUlevelL = sqrt(VUlevelL / ::BufferSize * 2); // кол. записей равно (BufferSize / 2)
		
		// правый канал
		VUlevelR = sqrt(VUlevelR / ::BufferSize * 2); // кол. записей равно (BufferSize / 2)

		
		// нормализация RMS (приводим в диапазон от 0 до 1)
		// максимум RMS для 8 бит = 127

		VUlevelL = VUlevelL / 127.0; // * 2.0 - было раньше;

		if (VUlevelL > 1.0)
			VUlevelL = 1.0;

		VUlevelR = VUlevelR / 127.0; // * 2.0 - было раньше;

		if (VUlevelR > 1.0)
			VUlevelR = 1.0;

	} // if (myBitsPerSample == 8)

	if (myBitsPerSample == 16) // глубина звука 16 бит
	{
		short* tek_pos = (short*)Buffer->data;

			// для указанного буфера вычисляем RMS
			for (i = 0; i < ::BufferSize; i += 4)
			{
				// левый канал
				yPosL = (double)tek_pos[0];

				VUlevelL += yPosL * yPosL;

				// правый канал
				yPosR = (double)tek_pos[1];

				VUlevelR += yPosR * yPosR;

				// смещаемся на 16*2 бита или 4 байта к следующему отсчёту
				tek_pos += 2; // тип short 2 байта
			}

			// левый канал
			VUlevelL = sqrt(VUlevelL / ::BufferSize * 4); // кол. записей равно (BufferSize/4)

			// правый канал
			VUlevelR = sqrt(VUlevelR / ::BufferSize * 4); // кол. записей равно (BufferSize/4)

			// нормализация RMS (приводим в диапазон от 0 до 1)
			// максимум RMS для 16 бит = 32767

			VUlevelL = VUlevelL / 32767.0; // * 2.0 - было раньше;

			if (VUlevelL > 1.0)
				VUlevelL = 1.0;

			VUlevelR = VUlevelR / 32767.0; // * 2.0 - было раньше;

			if (VUlevelR > 1.0)
				VUlevelR = 1.0;

	} // if (myBitsPerSample == 16)

	if (myBitsPerSample == 24) // глубина звука 24 бит
	{
		unsigned char* tek_pos = (unsigned char*)Buffer->data;

		// для указанного буфера вычисляем RMS
		for (i = 0; i < ::BufferSize; i += 6)
		{
			// левый канал
			yPosL = convertSampleToDouble(tek_pos);

			VUlevelL += yPosL * yPosL;

			// правый канал
			yPosR = convertSampleToDouble(tek_pos+3);

			VUlevelR += yPosR * yPosR;

			// смещаемся на на 24*2=48 бита или 6 байт
			tek_pos += 6;
		}

		// левый канал
		VUlevelL = sqrt(VUlevelL / ::BufferSize * 6); // кол. записей равно (BufferSize/6)

		// правый канал
		VUlevelR = sqrt(VUlevelR / ::BufferSize * 6); // кол. записей равно (BufferSize/6)

		// нормализация RMS (приводим в диапазон от 0 до 1)
		// максимум RMS для 24 бит = +1.0

		VUlevelL = VUlevelL / 1.0; // * 2.0 было раньше

		if (VUlevelL > 1.0)
			VUlevelL = 1.0;

		VUlevelR = VUlevelR / 1.0; // * 2.0 было раньше

		if (VUlevelR > 1.0)
			VUlevelR = 1.0;

	} // if (myBitsPerSample == 24)

	// вычисляем отображаемый уровень VU Meter
	{
		// левый канал
		{
			// если новое значение VU Meter меньше предыдущего,
			// то делаем затухание плавным
			if (VUlevelL < lastVUlevelL)
				lastVUlevelL = lastVUlevelL - 0.3 * (lastVUlevelL - VUlevelL); // экспоненциальное затухание
			// если новое значение VU Meter больше предыдущего,
			// то резкий рост разрешен
			else
				lastVUlevelL = VUlevelL;
		
		} // левый канал

		// правый канал
		{
			// если новое значение VU Meter меньше предыдущего,
			// то делаем затухание плавным
			if (VUlevelR < lastVUlevelR)
				lastVUlevelR = lastVUlevelR - 0.3 * (lastVUlevelR - VUlevelR); // экспоненциальное затухание
			// если новое значение VU Meter больше предыдущего,
			// то резкий рост разрешен
			else
				lastVUlevelR = VUlevelR;

		} // правый канал

	} // вычисляем отображаемый уровень VU Meter

	// отображаем уровень VU Meter
	{
		// левый канал
		yPosL = ((double)::glViewportHeight - 15.0) / (double)::glViewportHeight;  // от 0 до 1
		yPosL = yPosL * (glYmax - glYmin) + glYmin;

		// правый канал
		yPosR = ((double)::glViewportHeight / 2.0 - 15.0) / (double)::glViewportHeight;  // от 0 до 1
		yPosR = yPosR * (glYmax - glYmin) + glYmin;

		// фон VU Meter
		{
			// ширина окантовки VU Meter  = 200 + 4 пикселей (2 слева и 2 справа)

			// начальная точка
			xPos = ((double)::glViewportWidth - 212.0) / (double)::glViewportWidth; // от 0 до 1
			xPos = xPos * glXmax; // фактические координаты по оси X

			// конечная точка
			xPosEnd = ((double)::glViewportWidth - 8.0) / (double)::glViewportWidth; // от 0 до 1
			xPosEnd = xPosEnd * glXmax; // фактические координаты по оси X

			glColor3d(0.0, 0.0, 0.0); // цвет

			// левый канал
			DrawHorizontalLine(xPos, xPosEnd, yPosL, 14.0);

			// правый канал
			DrawHorizontalLine(xPos, xPosEnd, yPosR, 14.0);

			// начальная точка
			xPos = ((double)::glViewportWidth - 210.0) / (double)::glViewportWidth; // от 0 до 1
			xPos = xPos * glXmax; // фактические координаты по оси X

			// конечная точка
			xPosEnd = ((double)::glViewportWidth - 10.0) / (double)::glViewportWidth; // от 0 до 1
			xPosEnd = xPosEnd * glXmax; // фактические координаты по оси X

			glColor3d(0.4, 0.4, 0.4); // цвет

			// левый канал
			DrawHorizontalLine(xPos, xPosEnd, yPosL, 10.0);

			// правый канал
			DrawHorizontalLine(xPos, xPosEnd, yPosR, 10.0);

		} // фон VU Meter

		// VU Meter
		{
			glColor3d(0.0, 0.9, 0.0); // цвет (для левого и правого канала)

			// начальная точка (для левого и правого канала)
			xPos = ((double)::glViewportWidth - 210.0) / (double)::glViewportWidth; // от 0 до 1
			xPos = xPos * glXmax; // фактические координаты по оси X

			// левый канал
			{
				// конечная точка
				// максимальная ширина VU Meter 200 пикселей
				xPosEnd = ((lastVUlevelL * 200.0) + (double)::glViewportWidth - 210.0) / (double)::glViewportWidth; // от 0 до 1
				xPosEnd = xPosEnd * glXmax; // фактические координаты по оси X

				DrawHorizontalLine(xPos, xPosEnd, yPosL, 10.0);
			
			} // левый канал

			// правый канал
			{
				// конечная точка
				// максимальная ширина VU Meter 200 пикселей
				xPosEnd = ((lastVUlevelR * 200.0) + (double)::glViewportWidth - 210.0) / (double)::glViewportWidth; // от 0 до 1
				xPosEnd = xPosEnd * glXmax; // фактические координаты по оси X

				DrawHorizontalLine(xPos, xPosEnd, yPosR, 10.0);

			} // правый канал

		} // VU Meter

	} // отображаем уровень VU Meter

	
	// включаем эффект прозрачности (blending)
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// отображение шкал поверх VU Meter (от 0 до 100%)
	{
		// yPosL такое же как и у уровня VU Meter
		// 
		// yPosR такое же как и у уровня VU Meter

		glColor4d(0.0, 0.0, 0.0, 0.5); // чёрный цвет и полупрозрачный

		for (i = 5; i < 100; i += 5)
		{
			xPos = ((i * 2) + (double)::glViewportWidth - 210.0) / (double)::glViewportWidth; // от 0 до 1
			xPos = xPos * glXmax; // фактические координаты по оси X

			if (i == 50)
			{
				// левый канал
				DrawQuadPoint(xPos, yPosL, 4.0);
				// правый канал
				DrawQuadPoint(xPos, yPosR, 4.0);
			}
			else if (i % 10 == 0)
			{
				// левый канал
				DrawQuadPoint(xPos, yPosL, 2.0);
				// правый канал
				DrawQuadPoint(xPos, yPosR, 2.0);
			}
			else
			{
				// левый канал
				DrawQuadPoint(xPos, yPosL, 1.0);
				// правый канал
				DrawQuadPoint(xPos, yPosR, 1.0);
			}

		} // for (i = 5; i < 100; i += 5)

	} // отображение шкалы поверх VU Meter (от 0 до 100%)

	// Возвращаем прозрачность (blending) в исходное состояние
	glDisable(GL_BLEND);


	glPointSize(1); // возвращаем размер точки

}

void CleenRecBuffer(rec_list* &Head) {

	rec_list* Last;

	while (Head != NULL)
	{
		Last = Head->next;
		delete[]Head->data;
		delete Head;
		Head = Last;
	}
}

bool ChVolume(rec_list* Head, ULONG HeadSize, double volumeMultiplier)
{

	if (Head == NULL) 
		return false;

	if (volumeMultiplier == 1) 
		return false; // громкость не изменилась

	WCHAR s2[50];// буферные строки
	WCHAR s1[100]; // буферные строки

	// вывод в окно отчета
	wcscpy(s1, L"Новый уровень громкости - ");
	_itow((int)(volumeMultiplier*100.0), s2, 10);
	wcscat(s1, s2);
	wcscat(s1, L"% ..");
	ListBoxAddStr(hList1, s1);

	// изменяем громкость во всём буфере
	ULONG last_buffer_pos;

	// в зависимости от битрейта
	switch (::myBitsPerSample)
	{

	case 8:
	{
		UCHAR* buf; // ссылка на текущую запись
		double data1; // буферная переменная для преобразования

		rec_list* Last_buf = Head;

		while (Last_buf != NULL)
		{
			buf = (UCHAR*)Last_buf->data;

			for (last_buffer_pos = 0; last_buffer_pos < HeadSize; last_buffer_pos += 1)
			{
				data1 = (double)*buf;
				data1 -= 128.0; // 128 соответствует 0;
				data1 = round(data1 * volumeMultiplier); // ближайшее целое к выч. значению

				if (data1 < -128.0) *buf = 0;
				else
				{
					if (data1 > 127.0) *buf = 255;
					else *buf = (UCHAR)(data1 + 128.0);
				}

				buf++; // указатель сместить на 1 байт вправо
			}

			Last_buf = Last_buf->next;

		} // Last_buf != NULL

		ListBoxAddStr(hList1, L"Громкость успешно изменена!");
	}
	break;// 8 bit

	case 16:
	{
		short* buf; // ссылка на текущую запись
		double data1; // буферная переменная для преобразования

		rec_list* Last_buf = Head;

		while (Last_buf != NULL)
		{
			buf = (short*)Last_buf->data;

			for (last_buffer_pos = 0; last_buffer_pos < HeadSize ; last_buffer_pos += 2)
			{
				data1 = (double)*buf;
				data1 = round(data1 * volumeMultiplier); // ближайшее целое к выч. значению

				if (data1 < -32768.0) *buf = -32768;
				else
				{
					if (data1 > 32767.0) *buf = 32767;
					else *buf = (short)data1;
				}

				buf++; // указатель сместить 2 байта вправо
			}

			Last_buf = Last_buf->next;

		} // Last_buf != NULL

		ListBoxAddStr(hList1, L"Громкость успешно изменена!");
	}
	break; // 16 bit

	case 24:
	{
		char* buf; // ссылка на текущую запись
		int32_t data1; // буферная переменная для преобразования

		rec_list* Last_buf = Head;

		while (Last_buf != NULL)
		{
			buf = (char*)Last_buf->data;

			for (last_buffer_pos = 0; last_buffer_pos < HeadSize; last_buffer_pos += 3)
			{
				data1 = (static_cast<int32_t>(buf[2]) << 16) |  // MSB
					(static_cast<uint8_t>(buf[1]) << 8) |  // Middle Byte
					(static_cast<uint8_t>(buf[0]));         // LSB

				// для отрицательных чисел расширяем знак
				if (data1 & 0x800000) { // Если старший (из 24) бит знаковый
					data1 = data1 | 0xFF000000; // Заполняем старшие биты единицами
				}

				// увеличение/уменьшение громкости
				data1 = static_cast<int32_t>(round((double)data1 * volumeMultiplier));

				// если громкость вышла за пределы измерений в 24 бита, то ужимаем
				if (data1 < -8388608)
				{
					data1 = -8388608;
				}
				else 
				{
					if (data1 > 8388607)
						data1 = 8388607;
				}

				// возвращаем значение назад в массив данных
				buf[0] = static_cast<char>(data1 & 0xFF);        // LSB
				buf[1] = static_cast<char>((data1 >> 8) & 0xFF); // Middle Byte
				buf[2] = static_cast<char>((data1 >> 16) & 0xFF);// MSB

				buf += 3; // указатель сместить 3 байта вправо
			}

			Last_buf = Last_buf->next;

		} // Last_buf != NULL

		ListBoxAddStr(hList1, L"Громкость успешно изменена!");
	}
	break; // 24 bit

	default:
	{
		ListBoxAddStr(hList1, L"Громкость не изменена!");
		MessageBox(hWnd, L" Недопустимая глубина звучания!", L"Внимание!", MB_OK);
		return false;
	} // default:

	} // switch (::myBitsPerSample)

	return true; // громкость успешно изменена

}

// Обрезаем звук выше заданного порога звучания
void Clipping (rec_list *Head, ULONG HeadSize, double  ClippingMultiplier)
{
	if(Head == NULL)
		return;

	if(ClippingMultiplier < 0.01 || ClippingMultiplier >= 1.00) // от 1 до 99.9%
	{
		ListBoxAddStr(hList1, L"Недопустимый процент клиппирования!");
		return;
	}

	WCHAR s2[100];// буферные строки
	WCHAR s1[120]; // буферные строки

	// вывод в окно отчета
	wcscpy(s1, L"Граница клиппирования - ");
	_itow((int)(ClippingMultiplier * 100), s2, 10);
	wcscat(s1, s2);
	wcscat(s1, L"% от максимума..");
	ListBoxAddStr(hList1, s1);

	// счётчик внутри цикла
	ULONG last_buffer_pos;

	double MaxValue; // верхний (положительный) предел клиппинга
	double MinValue; // нижний (отрицательный) предел клиппинга

	switch (::myBitsPerSample)
	{

	case 8:
	{
		MaxValue = round(127.0 * ClippingMultiplier);
		MinValue = round(-128.0 * ClippingMultiplier);

		UCHAR* buf;
		double data1; // храним копию данных для преобразования

		rec_list* Last_buf = Head;

		while (Last_buf != NULL)
		{
			buf = (UCHAR*)Last_buf->data;

			for (last_buffer_pos = 0; last_buffer_pos < HeadSize; last_buffer_pos += 1)
			{
				data1 = (double)*buf;
				data1 -= 128.0; // приводим к диапазону -128..127

				if (data1 < MinValue)
				{
					*buf = (UCHAR)(MinValue + 128.0);
				}
				else
				{
					if (data1 > MaxValue)
					{
						*buf = (UCHAR)(MaxValue + 128.0);
					}
				}

				buf++; // указатель сместить на следующую позицию
			}

			Last_buf = Last_buf->next;

		} // while (Head != NULL)

		ListBoxAddStr(hList1, L"Клиппирование успешно выполнено!");

	} // case 8:
	break;

	case 16:
	{
		MaxValue = round(32767.0 * ClippingMultiplier);
		MinValue = round(-32768.0 * ClippingMultiplier);

		short* buf;
		double data1; // храним копию данных для преобразования

		rec_list* Last_buf = Head;

		while (Last_buf != NULL)
		{
			buf = (short*)Last_buf->data;

			for (last_buffer_pos = 0; last_buffer_pos < HeadSize; last_buffer_pos += 2)
			{
				data1 = (double)*buf;

				if (data1 < MinValue)
				{
					*buf = (short)MinValue;
				}
				else
				{
					if (data1 > MaxValue)
					{
						*buf = (short)MaxValue;
					}
				}

				buf++; // указатель сместить на следующую позицию (2 байта)
			}

			Last_buf = Last_buf->next;

		} // while (Last_buf != NULL)

		ListBoxAddStr(hList1, L"Клиппирование успешно выполнено!");

	} // case 16:
	break;

	case 24:
	{
		MaxValue = round(8388607.0 * ClippingMultiplier);
		MinValue = round(-8388608.0 * ClippingMultiplier);

		char* buf;
		int32_t data1; // храним копию данных для преобразования

		rec_list* Last_buf = Head;

		while (Last_buf != NULL)
		{
			buf = Last_buf->data;

			for (last_buffer_pos = 0; last_buffer_pos < HeadSize; last_buffer_pos += 3)
			{
				data1 = (static_cast<int32_t>(buf[2]) << 16) |  // MSB
					(static_cast<uint8_t>(buf[1]) << 8) |  // Middle Byte
					(static_cast<uint8_t>(buf[0]));         // LSB

				// для отрицательных чисел расширяем знак
				if (data1 & 0x800000)
				{ // Если старший (из 24) бит знаковый
					data1 = data1 | 0xFF000000; // Заполняем старшие биты единицами
				}

				if (data1 < MinValue)
				{
					data1 = (int32_t)MinValue;

					// возвращаем значение назад в массив данных
					buf[0] = static_cast<char>(data1 & 0xFF);        // LSB
					buf[1] = static_cast<char>((data1 >> 8) & 0xFF); // Middle Byte
					buf[2] = static_cast<char>((data1 >> 16) & 0xFF);// MSB
				}
				else
				{
					if (data1 > MaxValue)
					{
						data1 = (int32_t)MaxValue;

						// возвращаем значение назад в массив данных
						buf[0] = static_cast<char>(data1 & 0xFF);        // LSB
						buf[1] = static_cast<char>((data1 >> 8) & 0xFF); // Middle Byte
						buf[2] = static_cast<char>((data1 >> 16) & 0xFF);// MSB
					}
				}

				buf += 3; // указатель сместить на 3 байта вправо

			}

			Last_buf = Last_buf->next;

		} // while (Last_buf != NULL)

		ListBoxAddStr(hList1, L"Клиппирование успешно выполнено!");

	} // case 24:
	break;

	default:
	{
		ListBoxAddStr(hList1, L"Клиппирование не выполнено!");
		MessageBox(hWnd, L" Недопустимая глубина звучания!", L"Внимание!", MB_OK);
		return;
	} // default:

	} // switch (myBitsPerSample)

}
  
BOOL WaveRead(WCHAR* cFileName)
{

	WCHAR s1[300], s2[270]; // буферы текста
	FILE* file;
	int err;

	ListBoxAddStr(hList1, L"----------------------------------");

	err = _wfopen_s(&file, cFileName, L"rb");
	if (err) {
		ListBoxAddStr(hList1, L" Не удалось открыть файл:");
		wcscpy(s1, L" \"");
		wcsncat(s1, cFileName, 80);
		wcscat(s1, L"\"");
		ListBoxAddStr(hList1, s1);
		ListBoxAddStr(hList1, L"----------------------------------");
		return NULL;
	}

	else {
		ListBoxAddStr(hList1, L" Информация в файле: ");
	}

	// проверяем нужного ли формата файл
	DWORD   dwRIFF = 0;
	fread(&dwRIFF, sizeof(DWORD), 1, file);
	if (dwRIFF != MAKEFOURCC('R', 'I', 'F', 'F')) {
		ListBoxAddStr(hList1, L" Ошибка, Формат файла не RIFF!");
		fclose(file);
		ListBoxAddStr(hList1, L"----------------------------------");
		return NULL;
	}
	ListBoxAddStr(hList1, L" Формат файла RIFF");

	// пропускаем размер файла
	fseek(file, 4, SEEK_CUR); 

	DWORD       dwWAVE = 0;
	DWORD       dwFormat = 0;
	long        lSizeFmt = 0;

	// проверяем тип данных в файле
	fread(&dwWAVE, sizeof(DWORD), 1, file);
	if (dwWAVE != MAKEFOURCC('W', 'A', 'V', 'E')) {
		ListBoxAddStr(hList1, L" Ошибка, тип данных не WAVE!");
		fclose(file);
		ListBoxAddStr(hList1, L"----------------------------------");
		return NULL;
	}
	ListBoxAddStr(hList1, L" Тип данных WAVE");

	// проверяем наличие fmt-
	fread(&dwFormat, sizeof(DWORD), 1, file);
	if (dwFormat != MAKEFOURCC('f', 'm', 't', ' ')) {
		ListBoxAddStr(hList1, L" Ошибка, следующий идент. не fmt_ !");
		fclose(file);
		ListBoxAddStr(hList1, L"----------------------------------");
		return NULL;
	};
	ListBoxAddStr(hList1, L" Следующий идентификатор fmt_ ");

	// считываем WAVEFORMATEX и пропускаем остальную информацию
	fread(&lSizeFmt, sizeof(long), 1, file);
	if (lSizeFmt == 0) {
		ListBoxAddStr(hList1, L" Ошибка, не найдена инфо об аудио!");
		fclose(file);
		ListBoxAddStr(hList1, L"----------------------------------");
		return NULL;
	}
	wcscpy(s1, L" Размер аудио данных: ");
	_ltow(lSizeFmt, s2, 10);
	wcscat(s1, s2);
	wcscat(s1, L" байт");
	ListBoxAddStr(hList1, s1);

	// считываем WAVEFORMATEX
	WAVEFORMATEX    waveFormat = WAVEFORMATEX();
	fread((char*)&waveFormat, 16, 1, file);// считываем только 16 первых байт
	if (lSizeFmt > 16) fseek(file, lSizeFmt - 16, SEEK_CUR); // пропускаем остальную информацию
	//waveFormat.cbSize = 0; // размер доп. инфо = 0
	
	// выводим полученные данные о формате
	ListBoxAddStr(hList1, L"----------------------------------");
	wcscpy(s1, L" Формат файла (PCM=1): ");
	_itow((int)waveFormat.wFormatTag, s2, 10);
	wcscat(s1, s2);
	ListBoxAddStr(hList1, s1);

	if (waveFormat.wFormatTag != 1) {
		ListBoxAddStr(hList1, L" Ошибка, формат не PCM!");
		fclose(file);
		ListBoxAddStr(hList1, L"----------------------------------");
		return NULL;
	}

	wcscpy(s1, L" Кол-во каналов (MONO=1): ");
	_itow((int)waveFormat.nChannels, s2, 10);
	wcscat(s1, s2);
	ListBoxAddStr(hList1, s1);

	if (waveFormat.nChannels < 1 || waveFormat.nChannels>2) {
		ListBoxAddStr(hList1, L" Ошибка кол-ва каналов звука!");
		fclose(file);
		ListBoxAddStr(hList1, L"----------------------------------");
		return NULL;
	}

	wcscpy(s1, L" Частота дискретизации (Hz): ");
	_ltow(waveFormat.nSamplesPerSec, s2, 10);
	wcscat(s1, s2);
	ListBoxAddStr(hList1, s1);

	if (waveFormat.nSamplesPerSec > 192000 || waveFormat.nSamplesPerSec < 8000) {
		ListBoxAddStr(hList1, L" Ошибка Частоты дискретизации!");
		fclose(file);
		ListBoxAddStr(hList1, L"----------------------------------");
		return NULL;
	}

	wcscpy(s1, L" Глубина звука (bit): ");
	_itow((int)waveFormat.wBitsPerSample, s2, 10);
	wcscat(s1, s2);
	ListBoxAddStr(hList1, s1);

	if (waveFormat.wBitsPerSample > 32 || waveFormat.wBitsPerSample < 8) {
		ListBoxAddStr(hList1, L" Недопуст. глубина звука!");
		fclose(file);
		ListBoxAddStr(hList1, L"----------------------------------");
		return NULL;
	}

	wcscpy(s1, L" Средняя плотность (byte/sec): ");
	_ltow((int)waveFormat.nAvgBytesPerSec, s2, 10);
	wcscat(s1, s2);
	ListBoxAddStr(hList1, s1);

	wcscpy(s1, L" Размер семпла (byte/sample): ");
	_itow((int)waveFormat.nBlockAlign, s2, 10);
	wcscat(s1, s2);
	ListBoxAddStr(hList1, s1);

	wcscpy(s1, L" Размер доп. инфо (byte): ");
	_itow((int)waveFormat.cbSize, s2, 10);
	wcscat(s1, s2);
	ListBoxAddStr(hList1, s1);
	ListBoxAddStr(hList1, L"----------------------------------");

	// считываем блок 'fact' если он есть
	DWORD   dwNextBlock = 0;
	fread((char*)&dwNextBlock, sizeof(DWORD), 1, file);
	if (dwNextBlock == MAKEFOURCC('f', 'a', 'c', 't')) {

		ListBoxAddStr(hList1, L" Нашли блок 'fact', пропускаем его");

		DWORD   dwSizeFact = 0;
		fread((char*)&dwSizeFact, sizeof(DWORD), 1, file);

		char* buf1 = new char[dwSizeFact + 1];
		fread(buf1, sizeof(char), dwSizeFact, file);
		delete[] buf1;

		fread((char*)&dwNextBlock, sizeof(DWORD), 1, file);
	}

	if (dwNextBlock != MAKEFOURCC('d', 'a', 't', 'a')) {
		ListBoxAddStr(hList1, L" Ошибка! Не можем найти блок 'data'!");
		fclose(file);
		ListBoxAddStr(hList1, L"----------------------------------");
		return NULL;
	}
	else ListBoxAddStr(hList1, L" Нашли блок 'data' - всё OK :)");

	// считываем размер блока данных
	DWORD dwRazmWavData = 0;
	fread(&dwRazmWavData, sizeof(DWORD), 1, file);
	if (dwRazmWavData < 100) {
		ListBoxAddStr(hList1, L" Ошибка! Размер блока данных < 100!");
		fclose(file);
		ListBoxAddStr(hList1, L"----------------------------------");
		return NULL;
	}

	// копируем данные в структуру информации о буфере воспроизведения
	::struct_play_data = waveFormat;
	::struct_rec_data = waveFormat; // тоже на всякий случай

	// тут код для считывания самих данных
	// Заполним оставшиеся системные переменные
	::myChannels = struct_play_data.nChannels;
	::mySamplesPerSec = struct_play_data.nSamplesPerSec;
	::myBitsPerSample = struct_play_data.wBitsPerSample;

	// размер буфера равен примерно 1/20 секунды (8бит * 20 = 160 в знаменателе)
	switch (mySamplesPerSec) {
	case 11025: // 19 буферов в 1 секунде
		::BufferSize = 11040 * myChannels * myBitsPerSample / 160; // 160 = 8(бит/с)*20 - 1/20 секунды
		break;
	case 22050: // 19 буферов в 1 секунде
		::BufferSize = 22060 * myChannels * myBitsPerSample / 160;
		break;
	default: // а тут 20 буферов в 1 секунде (для всех прочих случаев)
		::BufferSize = mySamplesPerSec * myChannels * myBitsPerSample / 160;
	}

	RecBufferLenght_Mutex.lock(); // блокируем RecBufferLenght и AudioTimeMS в других потоках
	::RecBufferLenght = 0; // размер записи
	::AudioTimeMS = 0; // длина аудио в МС
	RecBufferLenght_Mutex.unlock(); // разблокируем RecBufferLenght и AudioTimeMS в других потоках

	PlayBufferPos_Mutex.lock(); // блокируем PlayBufferPos и PlayTimeMS в других потоках
	::PlayBufferPos = 0; // позиция воспроизведения
	::PlayTimeMS = 0; // таймер воспроизведения
	PlayBufferPos_Mutex.unlock(); // разблокируем PlayBufferPos и PlayTimeMS в других потоках

	mySoundGraph.Ready = false;

	// Очистим список в котором храним аудиоданные
	CleenRecBuffer(::Head_rec);
	// Head_rec = NULL; // уже реализовано в CleenRecBuffer()
	::Rec_Last_rec = NULL; // если идёт воспроизведение то оно останавливается
	
	::Head_rec = new rec_list;
	::Head_rec->data = new char[::BufferSize];
	rec_list* rlTemp = ::Head_rec;

	fread(rlTemp->data, sizeof(char), BufferSize, file);
	dwRazmWavData -= BufferSize;// размер оставшихся данных уменьшился

	RecBufferLenght_Mutex.lock(); // блокируем RecBufferLenght и AudioTimeMS в других потоках
	::RecBufferLenght += BufferSize;

	while (dwRazmWavData >= ::BufferSize) 
	{
		rlTemp->next = new rec_list;
		rlTemp = rlTemp->next;
		rlTemp->data = new char[::BufferSize];
		fread(rlTemp->data, sizeof(char), BufferSize, file);
		dwRazmWavData -= BufferSize;// размер оставшихся данных уменьшился
		::RecBufferLenght += BufferSize;
	}

	// в конце вычисляем длину аудио
	::AudioTimeMS = ::RecBufferLenght / ::mySamplesPerSec / (::myBitsPerSample / 8) / ::myChannels * 1000;
	RecBufferLenght_Mutex.unlock(); // разблокируем RecBufferLenght и AudioTimeMS в других потоках

	rlTemp->next = NULL; // вконце списка данных ссылка на след. элемент == NULL

	ListBoxAddStr(hList1, L" Успешно заполнили структуру данных!");

	// закрываем файл
	fclose(file);
	ListBoxAddStr(hList1, L"----------------------------------");

	// Посчитаем и выведем длительность воспроизведения
	double dDurationSeconds = AudioTimeMS / 1000;
	int iDurationMinutes = (int)floor(dDurationSeconds / 60);
	dDurationSeconds = dDurationSeconds - (iDurationMinutes * 60);
	wcscpy(s1, L"Длительность записи - ");
	_itow(iDurationMinutes, s2, 10);
	wcscat(s1, s2);
	wcscat(s1, L" мин. ");
	_itow((int)dDurationSeconds, s2, 10);
	wcscat(s1, s2);
	wcscat(s1, L" сек.");
	ListBoxAddStr(hList1, s1);

	// подготовка и визуализация аудиодорожки
	glVichMinMaxXY(Head_rec); // вычисляем границы звука для вывода его графика
	PrepareSoundGraph(Head_rec, BufferSize, RecBufferLenght);
	ShowAllSound = TRUE; // разрешаем визуализацию звука 
	InvalidateRect(hWnd, NULL, FALSE);  // заставляем окно перерисоватся

	// изменяем состояние кнопок
	EnableWindow(hBut2, FALSE); // стоп
	EnableWindow(hBut3, TRUE); // активируем кнопку воспроизведения
	EnableWindow(hBut1, TRUE);// активируем кнопку записи

	return TRUE;

}

BOOL WaveSave(WCHAR* cFileName, rec_list* Head, unsigned long lDataSize, unsigned long lBufSize, WAVEFORMATEX WaveInfo) {
	
	if (Head == NULL) 
	{
		ListBoxAddStr(hList1, L"Ошибка, нет данных для сохранения!");
		return NULL;
	}
	
	WCHAR s1[256], s2[256]; // буферы для текста

	FILE* file;

	int err;
	err = _wfopen_s(&file, cFileName, L"wb");

	if (err) {
		ListBoxAddStr(hList1, L"Ошибка, невозможно открыть файл для записи!");
		fclose(file);
		return NULL;
	}

	if (WaveInfo.nChannels == 0 || WaveInfo.nChannels > 2) {
		ListBoxAddStr(hList1, L"Ошибка, WaveInfo.nChannels = 0 или больше 2!");
		fclose(file);
		return NULL;
	}

	// поля в WAV файле в порядке их следования
	char cRIFF[5] = "RIFF"; // на 5 элементов так как в конце нуль символ
	long lRIFFSize = lDataSize + 36; // размер файла-8 байт
	char cWAVE[5] = "WAVE"; // на 5 элементов так как в конце нуль символ
	char cFmt_[5] = "fmt "; // на 5 элементов так как в конце нуль символ
	long lRazmWaveInfo = 16; // 16 для формата PCM
	//WAVEFORMATEX WaveInfo1 = WAVEFORMATEX(); // типа обнулили поля структыры
	char cData[5] = "data";

	// записываем все заголовочные поля и структуры
	fwrite(&cRIFF, sizeof(char), 4, file);
	fwrite(&lRIFFSize, sizeof(long), 1, file);
	fwrite(&cWAVE, sizeof(char), 4, file);
	fwrite(&cFmt_, sizeof(char), 4, file);
	fwrite(&lRazmWaveInfo, sizeof(long), 1, file);
	fwrite(&WaveInfo, 16, 1, file);// записываем только первые 16 байт структуры
	fwrite(&cData, sizeof(char), 4, file);
	fwrite(&lDataSize, sizeof(long), 1, file);

	// записываем наши аудиоданные с буфера в файл:
	ULONG MyDataSize = lDataSize;
	while (Head != NULL && (MyDataSize >= lBufSize)) 
	{
		// записываем данные с текущего буфера в файл
		if (fwrite(Head->data, sizeof(char), lBufSize, file) < lBufSize) 
		{
			ListBoxAddStr(hList1, L"Не удалось сохранить всю запись!");
			ListBoxAddStr(hList1, L"Пробуем закрыть файл...");
			if (!fclose(file))
				ListBoxAddStr(hList1, L"Файл успешно закрыт!");
			else
				ListBoxAddStr(hList1, L"Ошибка при закрытии файла!");
			
			return NULL;// не весь файл сохранен
		}
		Head = Head->next;
		MyDataSize -= lBufSize;
	}
	
	// сколько данных в файл не записалось
	wcscpy(s1, L" В буфере осталось ");
	_itow((int)MyDataSize, s2, 10);
	wcscat(s1, s2);
	wcscat(s1, L" байт");
	ListBoxAddStr(hList1, s1);

	// безопасно закрываем созданный нами файл
	if (!fclose(file))
	{
		ListBoxAddStr(hList1, L"Файл успешно сохранен!");
		return TRUE; // файл успешно сохранен
	}
	else
	{
		ListBoxAddStr(hList1, L"Ошибка при закрытии файла!");
		return NULL; // если файл не удалось успешно закрыть
	}

	
}

BOOL CutSound(rec_list* Head, unsigned long uStartBufPos, unsigned long uKolBuffers) 
{

	if (Head == NULL) 
	{
		MessageBox(hWnd, L" Массив звуковых данных пуст!     ", L"Ошибка функции CutSound", MB_OK | MB_ICONWARNING);
		return NULL;
	}

	// ищем стартовую позицию и удаляем с РАМ всё лишнее
	unsigned long i;

	rec_list* NewHead = Head;
	rec_list* LastPos;

	for (i = 0; i < uStartBufPos; i++)
	{
		delete[]NewHead->data; // освобождаем данные
		LastPos = NewHead->next;
		delete NewHead;
		NewHead = LastPos;
	}

	::Head_rec = NewHead;
	if (::Head_rec != NewHead)
	{
		MessageBox(hWnd, L"::Head != NewHead      ", L"Ошибка присваивания", MB_OK | MB_ICONWARNING);
		//return NULL;
	}

	// выбираем конечную позицию
	for (i = 0; i < uKolBuffers - 1; i++) {// если например с 1 по 5ю позицию то оставляем буферы с 1 по 4й не включая пятый
		NewHead = NewHead->next;
	}

	// изменяем размер буфера данных так как кол. данных уменьшилось (а также длительность аудио)
	RecBufferLenght_Mutex.lock(); // блокируем RecBufferLenght и AudioTimeMS в других потоках
	::RecBufferLenght = ::BufferSize * uKolBuffers; 
	::AudioTimeMS = ::RecBufferLenght / mySamplesPerSec / (myBitsPerSample / 8) / myChannels * 1000;
	RecBufferLenght_Mutex.unlock(); // разблокируем RecBufferLenght и AudioTimeMS в других потоках

	// все что дальше конечной позиции удаляем из памяти
	LastPos = NewHead->next;
	NewHead->next = NULL; // в конце обозначаем что массив с данными закончился
	
	while (LastPos != NULL)
	{
		delete[] LastPos->data;
		NewHead = LastPos->next;
		delete LastPos;
		LastPos = NewHead;
	}
	
	return TRUE;

}

LRESULT CALLBACK WindowFunc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps; // структура в которой рисуем окно OpenGL

	WCHAR s[250]; // буферные строки
	WCHAR s1[300]; // буферные строки

	// обработка сообщ.
	switch (msg)
	{

	// вызов перерисовки содержимого окна из другого потока через PostMessage
	case WM_USER + 1:
	{
		InvalidateRect(hWnd, NULL, FALSE);
		//Sleep(35);
	}
	break;

	// поток воспроизведения завершился (c ошибками или без)
	case WM_USER + 4:
	{
		// если поток раньше запускался, освобождаем ресурсы playThread
		if (playThread.joinable())
		{
			playThread.join();
		}

		isPlayThreadRunning.store(false); // поток воспроизведения остановлен

		// возвращаем состояние кнопок
		EnableWindow(hBut1, TRUE);// кнопка запись
		EnableWindow(hBut3, TRUE);// кнопка начала воспр
		EnableWindow(hBut5, FALSE);// кнопка остановки воспр.
		SetWindowText(hButPPause, L"Пауза"); //изменяем надпись кнопки "ВОСПР-ПАУЗА"
		EnableWindow(hButPPause, FALSE); // деактивируем кнопку паузы
		isNowPause = false; // кнопка паузы готова ставить на паузу

		// если поток выполнился и завершился с ошибками
		if (PlayResult.load() != 0)
		{
			MessageBox(hWnd, PlayMsgBoxErrString, L"Внимание", MB_OK | MB_ICONERROR);
			ListBoxAddStr(hList1, L"Устройство воспр. имеет ошибки");
		}
		else // поток выполнился и завершился без ошибок
		{
			ListBoxAddStr(hList1, L"Устройство воспр. успешно закрыто");
		}
		// изменить область просмотра OpenGL (для вывода всего звука)
		// и вызов перерисовки содержимого окна
		glVichMinMaxXY(Head_rec);
		InvalidateRect(hWnd, NULL, FALSE);

	} // поток воспроизведения завершился
	break;

	// поток воспроизведения запустился успешно
	case WM_USER + 5:
	{
		// меняем состояние кнопок
		EnableWindow(hButPPause, TRUE); // ПАУЗА
		EnableWindow(hBut5, TRUE); // СТОП вопроизведение

		// состояние остальных кнопок уже поменял в ID_START_PLAY

		ListBoxAddStr(hList1, L"Устройство воспр. успешно открыто");

	} // поток воспроизведения запустился успешно
	break;

	// поток записи: устройство записи успешно открылось
	case WM_USER + 10:
	{
		ListBoxAddStr(hList1, L"Устройство записи успешно открыто");
	}
	break;

	// поток записи: запись звука (или визуализация без сохранения) успешно стартовала
	case WM_USER + 11:
	{
		// запись и визуализация
		if (RecNoSave.load() == false)
		{
			ListBoxAddStr(hList1, L"Запись успешно стартовала...");
		}
		// только визуализация, без сохранения
		else
		{
			ListBoxAddStr(hList1, L"Визуализация успешно стартовала...");
		}
	}
	break;

	// поток записи: не удалось открыть устройство записи,
	// поток завершился
	case WM_USER + 12:
	{
		// Ждём, пока поток записи корректно завершится
		if (recThread.joinable())
		{
			recThread.join();
		}

		isRecThreadRunning.store(false); // помечаем, что поток уже остановлен

		MessageBox(hWnd, RecMsgBoxErrString, L"Внимание", MB_OK | MB_ICONERROR);

		ListBoxAddStr(hList1, L"Ошибка устройства записи");

		// кнопки старта и остановки записи
		EnableWindow(hBut1, TRUE); // запись
		EnableWindow(hBut2, FALSE); // стоп запись

		// кнопка воспроизведения выкл, так как звук удалили внутри ID_REC,
		// а новый новый записать не удалось
		EnableWindow(hBut3, FALSE); // воспроизведение

		// разблокируем поля настройки устройства записи
		EnableWindow(hComboBox1, TRUE); // выбор глубины звука
		EnableWindow(hComboBox2, TRUE); // выбор частоты дискретизации
		EnableWindow(hComboBox3, TRUE); // выбор СТЕРЕО или МОНО
		EnableWindow(hComboBox5, TRUE); // для выбора устройства записи

		// чекбокс предварительной визуализации
		RecNoSave.store(false); // выкл. предвариательную визуализацию
		// чекбокс предварительной визуализации делаем активным,
		// но галочка выключена
		EnableWindow(hCheckBoxNoSave, TRUE);
		SendMessage(hCheckBoxNoSave, BM_SETCHECK, BST_UNCHECKED, 0);

		// надпись "REC" скрываем (уведомляет о начале записи)
		ShowWindow(hWndRecSymbol, SW_HIDE);

		// запрещаем визуализацию всего графика звука (его удалили в ID_REC)
		ShowAllSound = FALSE;

		InvalidateRect(hWnd, NULL, FALSE);  // заставляем окно перерисоватся
	}
	break;

	// поток записи завершился (с ошибками или без)
	case WM_USER + 14:
	{
		EnableWindow(hBut2, FALSE); // кнопка СТОП

		// Ждём, пока поток записи корректно завершится
		if (recThread.joinable()) {
			recThread.join();
		}

		isRecThreadRunning.store(false); // поток уже остановлен

		// если поток записи завершился без ошибкок
		if (RecResult.load() == 0)
		{
			// если была только визуализация (без сохранения)
			if (RecNoSave.load() == true)
			{
				ListBoxAddStr(hList1, L"Визуализация успешно завершена!");
				RecNoSave.store(false); // выкл. предвариательную визуализацию
			}
			
			// если была запись и визуализация
			else
			{
				ListBoxAddStr(hList1, L"Запись успешно завершена!");
				EnableWindow(hBut1, TRUE);// активируем кнопку записи
				EnableWindow(hBut3, TRUE); // активируем кнопку воспроизведения
			}

			// если удалось что-то записать (была включена запись)
			if (Head_rec != NULL)
			{
				glVichMinMaxXY(Head_rec); // вычисляем границы звука для вывода его графика
				PrepareSoundGraph(Head_rec, BufferSize, RecBufferLenght);
				ShowAllSound = TRUE; // разрешаем визуализацию всего графика звука

				Rec_Last_rec = NULL;

			} // if (Head_rec != NULL)

			// была включена только предварительная визуализация
			else  // Head_rec == NULL - если ничего не записали
			{
				ShowAllSound = FALSE; // запрещаем график всего звука

				if (Rec_Last_rec != NULL)
				{
					delete[]Rec_Last_rec->data;
					delete Rec_Last_rec;
					Rec_Last_rec = NULL;
				}
			} // if Head_rec == NULL

		} // если поток записи завершился без ошибкок

		// если поток записи завершился с ошибками
		else
		{
			// если была только визуализация (без сохранения)
			if (RecNoSave.load() == true)
			{
				ListBoxAddStr(hList1, L"Визуализация завершена c ошибками!");
				RecNoSave.store(false); // выкл. предвариательную визуализацию
			}
			// если была запись и визуализация
			else
			{
				ListBoxAddStr(hList1, L"Запись завершена c ошибками!");
				EnableWindow(hBut1, TRUE);// активируем кнопку записи
				EnableWindow(hBut3, FALSE); // но отключаем кнопку воспроизведения
			}

			// выводим текст ошибки записи
			MessageBox(hWnd, RecMsgBoxErrString, L"Внимание", \
				MB_OK | MB_ICONERROR);

			// если была включена запись звука,
			// то очищаем весь список данных Head_rec
			if (Head_rec != NULL)
			{
				CleenRecBuffer(Head_rec);
				Rec_Last_rec = NULL;

			} // if (Head_rec != NULL)

			// если была только предварительная визуализация,
			// то чистим только Rec_Last_rec
			else // Head_rec == NULL
			{
				if (Rec_Last_rec != NULL)
				{
					delete[]Rec_Last_rec->data;
					delete Rec_Last_rec;
					Rec_Last_rec = NULL;
				}
			} // if Head_rec == NULL

			ShowAllSound = FALSE; // запрещаем график всего звука
		
		} // если поток записи завершился с ошибками

		// разблокируем поля настройки устройства записи
		EnableWindow(hComboBox1, TRUE); // выбор глубины звука
		EnableWindow(hComboBox2, TRUE); // выбор частоты дискретизации
		EnableWindow(hComboBox3, TRUE); // выбор СТЕРЕО или МОНО
		EnableWindow(hComboBox5, TRUE); // для выбора устройства записи

		// чекбокс предварительной визуализации делаем активным,
		// но галочка выключена
		EnableWindow(hCheckBoxNoSave, TRUE);
		SendMessage(hCheckBoxNoSave, BM_SETCHECK, BST_UNCHECKED, 0);

		// надпись "REC" скрываем (уведомляет о начале записи)
		ShowWindow(hWndRecSymbol, SW_HIDE);

		InvalidateRect(hWnd, NULL, FALSE);  // заставляем окно перерисоваться
	}
	break;

	case WM_CTLCOLORSTATIC: // задаём цвет фона для всех STATIC элементов
	{

		HDC hdcs = (HDC)wParam;
		HWND hw = (HWND)lParam;

		// для СТАТИКА hWndRecSymbol (надпись "REC") задаём цвет текста
		if (hw == hWndRecSymbol)
		{
			// красный текст
			SetTextColor(hdcs, RGB(255, 0, 0));
		}
		// Для всех остальных STATIC — чёрный текст
		else
		{
			SetTextColor(hdcs, RGB(0, 0, 0));
		}

		return (LRESULT)WhiteColorBrush; // цвет фона для STATIC элементов
	}

	case WM_LBUTTONDOWN:

	{
		if (isPlayThreadRunning.load() == true) // если сейчас проигрывается песня (или стоит на паузе)
		{
			int mouseX = LOWORD(lParam); // X-координата курсора
			int mouseY = HIWORD(lParam); // Y-координата курсора

			// если мышка внутри области OpenGL на уровне трекбара прогресса проигрывания
			if (mouseX > glViewportX && mouseX < glViewportX + glViewportWidth &&
				mouseY < wH - glViewportY && mouseY > wH - glViewportY - 20)
			{
				double PosX = mouseX - glViewportX; // координаты X внутри области
				PosX = PosX / glViewportWidth; // относительное положение от 0 до 1

				// --- меняем текущую позицию воспроизведения ---

				PlayBufferPos_Mutex.lock(); // блокируем PlayBufferPos и PlayTimeMS в других потоках

				// сохраняем новую позицию воспр. в локальную переменную Local_Last_rec
				// и сохраняем значение PlayBufferPos для прогресс бара воспроизведения
				rec_list* Local_Last_rec = GetAudioPosition(Head_rec, BufferSize, RecBufferLenght, \
					PosX, &PlayBufferPos);
				
				// меняем позицию воспроизведения (заносим в атомарную переменную Play_Last_rec)
				Play_Last_rec.store(Local_Last_rec, std::memory_order_release);

				// сохраняем значение PlayTimeMS для таймера воспроизведения
				PlayTimeMS = ::PlayBufferPos / ::mySamplesPerSec / (::myBitsPerSample / 8) / ::myChannels * 1000;
				
				PlayBufferPos_Mutex.unlock(); // разблокируем PlayBufferPos и PlayTimeMS в других потоках

				InvalidateRect(hWnd, NULL, FALSE);  // заставляем окно перерисоватся

			}// если мышка внутри области OpenGL на уровне трекбара прогресса проигрывания

		}// если сейчас проигрывается песня (или стоит на паузе)

	} // WM_LBUTTONDOWN
	break;

	case WM_MOUSEMOVE:

	{
		if (isPlayThreadRunning.load() == true) // если сейчас проигрывается песня (или стоит на паузе)
		{
			int mouseX = LOWORD(lParam); // X-координата курсора
			int mouseY = HIWORD(lParam); // Y-координата курсора

			// если мышка внутри области OpenGL на уровне трекбара прогресса проигрывания
			if (mouseX > glViewportX && mouseX < glViewportX + glViewportWidth &&
				mouseY < wH - glViewportY && mouseY > wH - glViewportY - 20)
			{
				SetCursor(hCursorHand); // курсор внутри области OpenGL
				break;
			} // если мышка внутри области OpenGL на уровне трекбара прогресса проигрывания
		} // если сейчас проигрывается песня (или стоит на паузе)

		SetCursor(hCursorArrow); // курсор по умолчанию

	} // WM_MOUSEMOVE
	break;

	case WM_CREATE:
	{
		// инициализация OpenGL
		hDC = GetDC(hWnd);
		SetWindowPixelFormat();
		hGLRC = wglCreateContext(hDC);
		wglMakeCurrent(hDC, hGLRC);

		// режим хранения точек
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		// начальные инициализации
		init();

		// делаем некоторые пункты главного меню в стиле RadioButton
		{
			// Получаем хэндл меню главного окна
			HMENU hMenu = GetMenu(hWnd);

			// Инициализируем структуру для изменения стиля пункта меню
			MENUITEMINFO mii = {};
			mii.cbSize = sizeof(MENUITEMINFO);
			mii.fMask = MIIM_FTYPE;
			mii.fType = MFT_STRING | MFT_RADIOCHECK;

			// Устанавливаем радио-стиль для двух пунктов
			SetMenuItemInfo(hMenu, ID_VIEW_GRAPH_QUICK_VISUALIZATION, FALSE, &mii);
			SetMenuItemInfo(hMenu, ID_VIEW_GRAPH_DETAILED_VISUALIZATION, FALSE, &mii);

			// Задаём начальный отмеченный пункт
			CheckMenuRadioItem(
				hMenu,
				ID_VIEW_GRAPH_QUICK_VISUALIZATION, // первый ID в группе
				ID_VIEW_GRAPH_DETAILED_VISUALIZATION, // последний ID в группе
				ID_VIEW_GRAPH_QUICK_VISUALIZATION, // отмечаем выбранный
				MF_BYCOMMAND);
		}

		// Раздел записи
		{
			// КНОПКИ
			hBut1 = CreateWindow(L"button", L" Запись ", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP, \
				1, 1, 1, 1, hWnd, (HMENU)ID_REC, hInst, NULL);

			hBut2 = CreateWindow(L"button", L" Стоп ", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP, \
				2, 1, 1, 1, hWnd, (HMENU)ID_STOP, hInst, NULL);

			// Предварительная вызуализация потока, ЧЕКБОКС
			hCheckBoxNoSave = CreateWindow(L"BUTTON", L"Предварительная визуализация",
				WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_CHECKBOX,
				3, 1, 1, 1, hWnd, (HMENU)ID_CHECKBOX_NOSAVE, hInst,
				NULL);

			// Выбор устройства записи (считывания) СПИСОК
			hComboBox5 = CreateWindow(L"ComboBox", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | \
				CBS_DROPDOWNLIST, 4, 1, 1, 1, hWnd, (HMENU)ID_COMBOBOX5, hInst, NULL); // выбор устр. записи

			// Глубина звука (бит)
			hComboBox1 = CreateWindow(L"ComboBox", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | \
				CBS_AUTOHSCROLL | CBS_DROPDOWNLIST, 5, 1, 1, 1, hWnd, (HMENU)ID_COMBOBOX1, hInst, NULL);

			// Частота дискретизации (Гц)
			hComboBox2 = CreateWindow(L"ComboBox", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | \
				CBS_DROPDOWNLIST, 6, 1, 1, 1, hWnd, (HMENU)ID_COMBOBOX2, hInst, NULL);

			// Количество каналов (МОНО/СТЕРЕО)
			hComboBox3 = CreateWindow(L"ComboBox", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | \
				CBS_DROPDOWNLIST, 7, 1, 1, 1, hWnd, (HMENU)ID_COMBOBOX3, hInst, NULL);

			// Рамка раздела записи 
			hGroupBox2 = CreateWindow(L"button", L"Устройство записи:", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
				8, 1, 1, 1, hWnd, (HMENU)ID_GROUPBOX2, hInst, NULL);
		
		} // Раздел записи

		// Раздел воспроизведения
		{
			// КНОПКИ
			hBut3 = CreateWindow(L"button", L"Воспр.", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP, \
				1, 2, 1, 1, hWnd, (HMENU)ID_START_PLAY, hInst, NULL); // в секции воспроизведении

			hButPPause = CreateWindow(L"button", L"Пауза", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP, \
				2, 2, 1, 1, hWnd, (HMENU)ID_PAUSE_PLAY, hInst, NULL); // в секции воспроизведении

			hBut5 = CreateWindow(L"button", L"Стоп", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP, \
				3, 2, 1, 1, hWnd, (HMENU)ID_STOP_PLAY, hInst, NULL); // в секции воспроизведении

			// Рамка раздела воспроизведения
			hGroupBox3 = CreateWindow(L"button", L"Устройство воспроизведения:", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
				4, 2, 1, 1, hWnd, (HMENU)ID_GROUPBOX3, hInst, NULL);

		} // Раздел воспроизведения

		// Окно вывода диагностической информации + его элементы
		{
			// окошко списка
			hList1 = CreateWindow(L"listbox", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | LBS_NOSEL,
				1, 3, 1, 1, hWnd, (HMENU)ID_LIST1, hInst, NULL);

			// добавляем эффект объёмных рамок для списка hList1
			LONG_PTR hList1exStyle = GetWindowLongPtr(hList1, GWL_EXSTYLE); // Получаем текущие расширенные стили hList1 
			hList1exStyle |= WS_EX_CLIENTEDGE; // Добавляем стиль WS_EX_CLIENTEDGE для объёмной рамки
			SetWindowLongPtr(hList1, GWL_EXSTYLE, hList1exStyle); // Применяем изменения
			
			//ListBoxAddStr(hList1, "Оччень длинная текстовая строка, которую мы будем добавлять в список и проверять, отображается ли она");

			// Очистка hList1, КНОПКА
			hBut4 = CreateWindow(L"button", L"Очистить", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP, \
				2, 3, 1, 1, hWnd, (HMENU)ID_RESET_LIST, hInst, NULL);

			// включение/выключение визуализации для hList1 и hBut4
			hCheckBoxShowMessageLog = CreateWindow(L"BUTTON", L"Показать журнал событий",
				WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_CHECKBOX,
				3, 3, 1, 1, hWnd, (HMENU)ID_CHECKBOX_SHOW_MESSAGE_LOG, hInst,
				NULL);

			PostMessage(hCheckBoxShowMessageLog, BM_SETCHECK, BST_CHECKED, 0);
		
		} // Окно вывода диагностической информации + его элементы

		// Раздел обрезки звука
		{
			//Трекбары
			hTrBar1 = CreateWindow(TRACKBAR_CLASS, L"имя трекбара 1",
				WS_CHILD | WS_VISIBLE | TBS_ENABLESELRANGE | TBS_FIXEDLENGTH,
				5, 125, wW - 10, 25, hWnd, (HMENU)ID_TRBAR1, hInst, NULL);
			hTrBar2 = CreateWindow(TRACKBAR_CLASS, L"имя трекбара 2",
				WS_CHILD | WS_VISIBLE | TBS_ENABLESELRANGE | TBS_FIXEDLENGTH,
				5, 150, wW - 10, 25, hWnd, (HMENU)ID_TRBAR2, hInst, NULL);

			// Устанавливаем диапазон в трекбаре от 0 до 10000 
			SendMessage(hTrBar1, TBM_SETRANGE, FALSE, MAKELONG(0, 10000));
			SendMessage(hTrBar2, TBM_SETRANGE, FALSE, MAKELONG(0, 10000));

			// Устанавливае положения регуляторов трекбаров
			SendMessage(hTrBar1, TBM_SETPOS, TRUE, 0); // трекбар 1 край слева
			SendMessage(hTrBar2, TBM_SETPOS, TRUE, 10000); // трекбар 2 край справа

			// КНОПКА "Обрезать звук"
			hButCutSound = CreateWindow(L"button", L"_Обрезать звук_", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP,
				1, 4, 1, 1, hWnd, (HMENU)ID_SOUND_CUT, hInst, NULL);

		} // Раздел обрезки звука

		// Прочие элементы
		{
			// вывод "длительность записи звука" или "время воспроизведения / общая длина звука"
			hWndTime = CreateWindow(L"STATIC", L" 00:00:00 / 00:00:00", WS_CHILD | WS_VISIBLE | SS_CENTER /* | WS_BORDER*/,
				1, 5, 1, 1, hWnd, (HMENU)ID_STATIC_SOUND_TIME, hInst, NULL);

			// выводим надпись "REC" внизу слева в окне
			hWndRecSymbol = CreateWindow(L"STATIC", L"REC", WS_CHILD | WS_VISIBLE | SS_ICON | SS_CENTERIMAGE /* | WS_BORDER*/,
				2, 5, 64, 24, hWnd, (HMENU)ID_STATIC_REC_SYMBOL, hInst, NULL);
			
			// вместо текста "REC" выводим иконку
			HICON hRecIcon = NULL;
			hRecIcon = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON_REC_8BIT),
				IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_SHARED); // убрать LR_SHARED если будем потом менять иконку внутри hWndRecSymbol
			if (hRecIcon)
			{
				// ставим иконку в контрол
				SendMessage(hWndRecSymbol, STM_SETICON, (WPARAM)hRecIcon, 0);
			}

			// скрываем надпись "REC" пока не начали запись звука
			ShowWindow(hWndRecSymbol, SW_HIDE);

		} // Прочие элементы

		// Вначале все кнопки кроме старта записи неактивны
		EnableWindow(hBut1, TRUE);
		EnableWindow(hBut2, FALSE);
		EnableWindow(hBut3, FALSE);
		EnableWindow(hButPPause, FALSE);
		EnableWindow(hBut5, FALSE);

		// Добавляем данные о глубине звука, частоте дискретизации, кол. каналов
		{
			SendMessage(hComboBox1, CB_ADDSTRING, 0, (LPARAM)L" 8 bit");
			SendMessage(hComboBox1, CB_ADDSTRING, 0, (LPARAM)L"16 bit");
			SendMessage(hComboBox1, CB_ADDSTRING, 0, (LPARAM)L"24 bit");
			SendMessage(hComboBox1, CB_SETCURSEL, 1, NULL); // выбрать вторую глубину (16)

			SendMessage(hComboBox2, CB_ADDSTRING, 0, (LPARAM)L"8000 Hz");
			SendMessage(hComboBox2, CB_ADDSTRING, 0, (LPARAM)L"11025 Hz");
			SendMessage(hComboBox2, CB_ADDSTRING, 0, (LPARAM)L"22050 Hz");
			SendMessage(hComboBox2, CB_ADDSTRING, 0, (LPARAM)L"44100 Hz");
			SendMessage(hComboBox2, CB_ADDSTRING, 0, (LPARAM)L"48000 Hz");
			SendMessage(hComboBox2, CB_ADDSTRING, 0, (LPARAM)L"96000 Hz");
			SendMessage(hComboBox2, CB_ADDSTRING, 0, (LPARAM)L"192000 Hz");
			SendMessage(hComboBox2, CB_SETCURSEL, 3, NULL); // выбрать частоту дискр. 44100

			SendMessage(hComboBox3, CB_ADDSTRING, 0, (LPARAM)L"Mono  ");
			SendMessage(hComboBox3, CB_ADDSTRING, 0, (LPARAM)L"Stereo");
			SendMessage(hComboBox3, CB_SETCURSEL, 0, NULL);// выбираем режим "Моно"
		}

		// Добавляем устройства записи в hComboBox5
		{
			SendMessage(hComboBox5, CB_ADDSTRING, 0, (LPARAM)L"Устройство записи по умолчанию");
			SendMessage(hComboBox5, CB_SETCURSEL, 0, NULL);// выбираем Устройство записи по умолчанию (номер ноль)
			waveInDevNummer = WAVE_MAPPER; // выбираем Устройство записи по умолчанию

			int waveInDevLength = 0; // количество устройств для записи в системе

			wcscpy(s1, L"Количество устройств записи - ");
			waveInDevLength = waveInGetNumDevs(); // узнаём кол-во устройств записи
			_itow(waveInDevLength, s, 10);
			wcscat(s1, s);
			ListBoxAddStr(hList1, s1); // выводим в список инфо о количестве устр.записи

			if (waveInDevLength > 0 && waveInDevLength < 100)
			{
				// для вывода информации об устройстве записи
				WAVEINCAPS  wcapsInCaps;

				for (UINT iii = 0; iii < waveInDevLength; iii++)
				{
					waveInGetDevCaps(iii, &wcapsInCaps, sizeof(WAVEINCAPS)); // узнаём инфу о конкретном устр.
					//выводим имя устройства в hComboBox5
					SendMessage(hComboBox5, CB_ADDSTRING, 0, (LPARAM)wcapsInCaps.szPname);
				}

			} // if (waveInDevLength > 0 && waveInDevLength < 100)

		} // Добавляем устройства записи в hComboBox5

	} // WM_CREATE
	break;

	case WM_PAINT:
	{
		// вычисляем время выполнения одного прохода WM_PAINT
		//auto start = std::chrono::high_resolution_clock::now();

		HDC MyHDC = BeginPaint(hWnd, &ps);

		// перерисовываем вручную фон окна (теперь он у нас сам не перерисовывается)
		glClearColor(1, 1, 1, 1); // // цвет очистки
		glClear(GL_COLOR_BUFFER_BIT);

		// обновляем в главном окне время записи/воспроизведения
		RecBufferLenght_Mutex.lock(); // блок. RecBufferLenght и AudioTimeMS в других потоках
		double localAudioTimeMS = AudioTimeMS;
		RecBufferLenght_Mutex.unlock(); // блок. RecBufferLenght и AudioTimeMS в других потоках

		PlayBufferPos_Mutex.lock(); // блокируем PlayBufferPos и PlayTimeMS в других потоках
		double localPlayBufferPos = PlayBufferPos;
		double localPlayTimeMS = PlayTimeMS;
		PlayBufferPos_Mutex.unlock(); // разблокируем PlayBufferPos и PlayTimeMS в других потоках

		ShowAudioTime(hWndTime, localAudioTimeMS, PlayTimeMS);

		// перерисовываем область вывода OpenGL
		display(localPlayBufferPos);

		// если выполняем запись, то меняем стиль шрифта для надписи "REC"
		/*
		if (isRecThreadRunning.load() == true && RecNoSave.load() == false)
		{
			static DWORD dwLastCall = GetTickCount(); // время последней смены шрифта

			static bool BoldFont = true; // рисуем жирный или обычный шрифт

			DWORD dwNow = GetTickCount(); // текущее время

			// 1 раз в секунду меняем стиль шрифта для надписи "REC"
			if (dwNow - dwLastCall > 1000)
			{
				dwLastCall = dwNow;

				if (BoldFont)
				{
					SendMessage(hWndRecSymbol, WM_SETFONT, (WPARAM)hFontRecSymbol, TRUE);
					BoldFont = false; // меняем стиль шрифта
				}
				else
				{
					SendMessage(hWndRecSymbol, WM_SETFONT, (WPARAM)hFontButton, TRUE);
					BoldFont = true; // меняем стиль шрифта
				}
			}

		} // если выполняем запись
		*/
		
		// если выполняем запись, то рисуем внизу слева значок записи
		// надпись затирается при дискретной видеокарте (Nvidia), не используем
		/*
		if (isRecThreadRunning.load() == true && RecNoSave.load() == false)
		{
			// яркость значка записи (от 128 до 255)
			static short int ColorBright = 128; // инициализация только 1 раз
			// направление яркости (true - увеличиваем яркость, false - уменьшаем)
			static bool ColorDirection = true; // инициализация только 1 раз
			if (ColorDirection) // увеличиваем яркость
			{
				ColorBright = ColorBright + 10;
				if (ColorBright > 255)
				{
					ColorBright = 255;
					ColorDirection = false;
				}
			}
			else // уменьшаем яркость
			{
				ColorBright = ColorBright - 10;
				if (ColorBright < 128)
				{
					ColorBright = 128;
					ColorDirection = true;
				}
			}
			
			// значёк записи, размеры и положение
			static int size = 15; // инициализация только 1 раз
			static int x0 = 15; // инициализация только 1 раз
			int y0 = ps.rcPaint.bottom - size - 15;

			// устанавливаем цвет кисти для этого DC
			SetDCBrushColor(MyHDC, RGB(ColorBright, 0, 0));

			// берём эту DC-кисть из стоковых объектов
			HBRUSH hBrush = (HBRUSH)GetStockObject(DC_BRUSH);

			// выбираем эту кисть (и сохраняем предыдущую кисть)
			HBRUSH hOldBr = (HBRUSH)SelectObject(MyHDC, hBrush);

			// рисуем значёк записи (красный эллипс)
			Ellipse(MyHDC, x0, y0, x0 + size, y0 + size);

			// восстанавливаем предыдущую кисть
			SelectObject(MyHDC, hOldBr);
		}
		*/


		EndPaint(hWnd, &ps);

		// вычисляем время выполнения одного прохода WM_PAINT
		////auto end = std::chrono::high_resolution_clock::now();
		// Подсчёт времени выполнения в микросекундах
		////auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
		// Время выполнения в переменной (в микросекундах)
		////long long elapsed_time_us = duration.count();
		/////

	} // WM_PAINT
	break;

	case WM_GETMINMAXINFO:
	{
		if (WindowDPI < 1)
			WindowDPI = GetEffectiveDpi(hWnd);

		// пытаемся получить разрешение текущего монитора
		int MonitorW = 0, MonitorH = 0;
		{
			HMONITOR hMon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
			MONITORINFO mi;
			mi.cbSize = sizeof(mi);

			if (hMon != NULL && GetMonitorInfo(hMon, &mi))
			{
				RECT work = mi.rcWork;

				MonitorW = work.right - work.left;
				MonitorH = work.bottom - work.top;
			}

			// Была ошибка при вызове MonitorFromWindow() или GetMonitorInfo()
			else
			{
				// Диагностика (опционально)
				// DWORD err = GetLastError();

				// пытаемся определить рабочую область основного экрана
				RECT rcWork;

				if (SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWork, 0))
				{
					MonitorW = rcWork.right - rcWork.left;
					MonitorH = rcWork.bottom - rcWork.top;
				}

				// Была ошибка при вызове SystemParametersInfo()
				else
				{
					// окончательный fallback на полные размеры экрана
					MonitorW = GetSystemMetrics(SM_CXSCREEN);
					MonitorH = GetSystemMetrics(SM_CYSCREEN);
				}

			} // Была ошибка при вызове MonitorFromWindow() или GetMonitorInfo()

		} // пытаемся получить разрешение текущего монитора

		// определяем текущий масштаб в системе
		float Scale = (float)WindowDPI / 96.0f; // масштаб окна (в Win XP равен 1.0)

		// зная разрешение монитора и масштаб в системе задаём мин размер окна
		{
			int MinWindowW, MinWindowH;

			// по умолчанию мин разрешение окна = 600x400 * масштаб
			MinWindowW = int(600.0f * Scale + 0.5f); // крайний правый елемент = 580 (+ 20 пикселей)
			MinWindowH = int(400.0f * Scale); // весь интерфейс = 260 (без области OpenGL, меню, шапки)

			// если разрешения монитора слишком низкое, 
			// то мин. размер окна = разрешение монитора - 30 пикселей
			if (MinWindowW > MonitorW - 30 || MinWindowH > MonitorH - 30)
			{
				MinWindowW = MonitorW - 30;
				MinWindowH = MonitorH - 30;
			}

			MINMAXINFO* pInfo = (MINMAXINFO*)lParam;

			POINT ptMin = { MinWindowW, MinWindowH };

			pInfo->ptMinTrackSize = ptMin;

		}

	} // WM_GETMINMAXINFO
	break;

	case WM_SIZE:
	{
		// храним в глобальных переменных размеры окна
		wW = LOWORD(lParam);
		wH = HIWORD(lParam);

		float Scale = (float)WindowDPI / 96.0f; // масштаб окна (в Win XP равен 1.0)

		// перерисовываем положение всех элементов окна
		RedrawWindowElements(wW, wH, Scale);

	} // WM_SIZE
	break;

	case 0x02E0: // WM_DPICHANGED:
	{
		UINT newDpi = LOWORD(wParam);

		if (newDpi == 0) newDpi = HIWORD(wParam); // запасной вариант
		if (newDpi == 0) newDpi = GetEffectiveDpi(hWnd); // крайний fallback

		WindowDPI = newDpi; // получаем новый DPI окна

		RECT* prc = (RECT*)lParam; // рекомендуемые Windows размеры окна при смене DPI

		if (prc) // prc != 0
		{
			// меняем размеры и расположение окна согласно рекомендациям Windows 
			SetWindowPos(hWnd, NULL,
				prc->left, prc->top,
				prc->right - prc->left, prc->bottom - prc->top,
				SWP_NOZORDER | SWP_NOACTIVATE);

		} // prc != 0

		// получаем текущие размеры клиентской области
		RECT rc;
		GetClientRect(hWnd, &rc);

		// сохраняем новые размеры окна в глобальные переменные
		wW = rc.right - rc.left;
		wH = rc.bottom - rc.top;

		// prc == 0

		float Scale = (float)WindowDPI / 96.0f; // масштаб окна (в Win XP равен 1.0)

		// перерисовываем положение всех элементов окна
		RedrawWindowElements(wW, wH, Scale);

		// заставляем окно сразу перерисоватся
		InvalidateRect(hWnd, NULL, FALSE);
		UpdateWindow(hWnd);

		ListBoxAddStr(hList1, L"Масштаб окна изменился (вызов WM_DPICHANGED)");

	} // WM_DPICHANGED:
	break;

	case WM_COMMAND: // обработка работы меню и кнопок

		switch (LOWORD(wParam)) // кнопки
		{

		// очистить список hList1
		case ID_RESET_LIST:
		{
			// !!!!! временный код
			{
				// сравнение разных стилей отображения
				if ((GetKeyState(VK_CONTROL) & 0x8000) != 0) // кнопка + CTRL
				{
					// если данные о графе заполнены
					if (mySoundGraph.Ready)
					{
						MENUITEMINFO mii = {};
						mii.cbSize = sizeof(MENUITEMINFO);
						mii.fMask = MIIM_STATE;

						// узнаём состояние пункта меню "Быстрая визуализация"
						GetMenuItemInfo(GetMenu(hWnd), ID_VIEW_GRAPH_QUICK_VISUALIZATION, FALSE, &mii);

						// если сейчас вкл "Быстрая визуализация" то меняем на "Детальная визуализация"
						if (mii.fState & MFS_CHECKED)
						{
							CheckMenuRadioItem(
								GetMenu(hWnd),
								ID_VIEW_GRAPH_QUICK_VISUALIZATION, // первый ID в группе
								ID_VIEW_GRAPH_DETAILED_VISUALIZATION, // последний ID в группе
								ID_VIEW_GRAPH_DETAILED_VISUALIZATION, // отмечаем выбранный стиль
								MF_BYCOMMAND);

							mySoundGraph.Show = false;
						}
						// если сейчас вкл "Детальная визуализация" то меняем на "Быстрая визуализация"
						else
						{
							CheckMenuRadioItem(
								GetMenu(hWnd),
								ID_VIEW_GRAPH_QUICK_VISUALIZATION, // первый ID в группе
								ID_VIEW_GRAPH_DETAILED_VISUALIZATION, // последний ID в группе
								ID_VIEW_GRAPH_QUICK_VISUALIZATION, // отмечаем выбранный стиль
								MF_BYCOMMAND);

							mySoundGraph.Show = true;
						}
					}
					// если данные о графе не заполнены
					else
					{
						CheckMenuRadioItem(
							GetMenu(hWnd),
							ID_VIEW_GRAPH_QUICK_VISUALIZATION, // первый ID в группе
							ID_VIEW_GRAPH_DETAILED_VISUALIZATION, // последний ID в группе
							ID_VIEW_GRAPH_DETAILED_VISUALIZATION, // отмечаем выбранный стиль
							MF_BYCOMMAND);

						mySoundGraph.Show = false;
					}

					InvalidateRect(hWnd, NULL, FALSE);  // заставляем окно перерисоватся

					break; // список уже не очищаем
				}

				// имитируем нажитие на пункт "Отображать клиппинг"
				if ((GetKeyState(VK_SHIFT) & 0x8000) != 0) // кнопка + SHIFT
				{
					PostMessage(hWnd, WM_COMMAND, ID_VIEW_GRAPH_SHOW_OVERLOAD, 0);

					break; // список уже не очищаем
				}
			}
			// !!!!! КОНЕЦ временного кода

			// если просто нажали кнопку, то очистим список hList1
			SendMessage(hList1, LB_RESETCONTENT, 0, 0L);

		} // ID_RESET_LIST
		break;

		// вкл/выкл отображение hList1 и hBut4
		case ID_CHECKBOX_SHOW_MESSAGE_LOG:
		{
			// нажали на чекбокс
			if (HIWORD(wParam) == BN_CLICKED)
			{
				// Если галочка ещё не стояла
				if (SendMessage(hCheckBoxShowMessageLog, BM_GETCHECK, 0, 0) == BST_UNCHECKED)
				{
					// ставим галочку
					SendMessage(hCheckBoxShowMessageLog, BM_SETCHECK, BST_CHECKED, 0);

					ShowWindow(hList1, SW_SHOW);
					ShowWindow(hBut4, SW_SHOW);

				}

				// Если галочка уже стояла
				else
				{
					// убираем галочку
					SendMessage(hCheckBoxShowMessageLog, BM_SETCHECK, BST_UNCHECKED, 0);

					ShowWindow(hList1, SW_HIDE);
					ShowWindow(hBut4, SW_HIDE);

				}
			}

		} // ID_CHECKBOX_SHOW_MESSAGE_LOG
		break;

		// начать запись
		case ID_REC:
		{
			// если поток записи уже запущен
			if(isRecThreadRunning.load() == true)
			{
				// была включена предварительная визуализация
				// сохранение было ВЫКЛЮЧЕНО, значит ВКЛЮЧАЕМ
				if (RecNoSave.load() == true)
				{
					// начинаем реальную запись!

					// включаем сохранение записи!
					RecNoSave.store(false);

					// меняем состояние кнопок и чекбокса
					EnableWindow(hBut1, FALSE); // запись
					EnableWindow(hBut2, TRUE); // стоп запись
					EnableWindow(hBut3, FALSE); // воспроизведение

					// снимаем галочку с чекбокса предв. визуализации
					SendMessage(hCheckBoxNoSave, BM_SETCHECK, BST_UNCHECKED, 0);
					EnableWindow(hCheckBoxNoSave, FALSE); // чекбокс делаем неактивным

					// надпись "REC" уведомляет о начале записи
					ShowWindow(hWndRecSymbol, SW_SHOW);

				}
				// не была включена предварительная визуализация
				// сообщение об ошибке (кнопка должна была быть неактивна)
				else
				{
					MessageBox(hWnd, L"Запись уже запущена!", L"Ошибка", MB_OK);
					break;
				}

			} // если поток записи уже запущен

			// если поток записи ещё не запущен
			else
			{
				// считываем с Combobox данные о формате записи и воспроизведения
				ChangeFormatRec();

				// старую аудиоапись в любом случае надо удалить, 
				// так как уже поменялся формат записи/воспр.

				ShowAllSound = FALSE; // запрещаем визуализацию графика всего аудиофайла

				mySoundGraph.Ready = false; // график всего звука в сжатом виде

				// освобождаем память, если данные в ней уже были
				CleenRecBuffer(Head_rec);

				// данные уже удалены, воспроизведение запрещаем
				EnableWindow(hBut3, FALSE); // кнопка воспроизведения

				RecBufferLenght_Mutex.lock(); // блокируем RecBufferLenght и AudioTimeMS в других потоках

				::RecBufferLenght = 0; // длина записи пока равна нулю
				::AudioTimeMS = 0; // длина аудио пока равна нулю

				RecBufferLenght_Mutex.unlock(); // разблокируем RecBufferLenght и AudioTimeMS

				//тестовое открытие устройства записи
				// если нет ошибок, то можно использовать для записи
				MMRESULT rc; // храним значение, возвращаемое функцией
				HWAVEIN hTemp = NULL;

				rc = waveInOpen(&hTemp, waveInDevNummer, &struct_rec_data, 0, 0, CALLBACK_NULL);
				if (rc != MMSYSERR_NOERROR)
				{
					// выключить чекбокс предварительной визуализации
					if (RecNoSave.load() == true)
					{
						RecNoSave.store(false);
						//EnableWindow(hCheckBoxNoSave, TRUE);
						SendMessage(hCheckBoxNoSave, BM_SETCHECK, BST_UNCHECKED, 0);
					}

					if (rc == MMSYSERR_BADDEVICEID)
					{
						MessageBox(hWnd, \
							L"Устройства записи с данным ID не существует.\nПопробуйте выбрать другое устройство.",
							L"Ошибка", MB_OK | MB_ICONERROR);

						InvalidateRect(hWnd, NULL, FALSE);  // заставляем окно перерисоватся

						break;
					}

					if (rc == WAVERR_BADFORMAT)
					{
						MessageBox(hWnd, \
							L"Устройство записи не поддерживает выбранные настройки.\nПопробуйте изменить параметры записи.",
							L"Ошибка", MB_OK | MB_ICONERROR);

						InvalidateRect(hWnd, NULL, FALSE);  // заставляем окно перерисоватся

						break;
					}

					else
					{
						// текст с описанием ошибки
						wchar_t errText[310] = L"Невозможно открыть устройство записи.\nТекст ошибки:\n\"";
						waveInGetErrorText(rc,errText + wcslen(errText),\
							310 - wcslen(errText));
						wcscat(errText, L"\"");

						MessageBox(hWnd, errText, L"Ошибка", MB_OK | MB_ICONERROR);

						InvalidateRect(hWnd, NULL, FALSE);  // заставляем окно перерисоватся

						break;
					}
				}

				// закрываем устройство после успешного теста
				waveInClose(hTemp);

				// если запись и визуализация
				if (RecNoSave.load() == false)
				{
					// начинаем реальную запись
					
					// меняем состояние кнопок
					EnableWindow(hBut1, FALSE); // запись
					EnableWindow(hBut2, TRUE); // стоп запись

					// чекбокс делаем неактивным
					EnableWindow(hCheckBoxNoSave, FALSE);

					// надпись "REC" уведомляет о начале записи
					ShowWindow(hWndRecSymbol, SW_SHOW);
				}

				// если предварительная визуализация (без сохранения)
				else
				{}

				// блокируем поля настройки устройства записи
				EnableWindow(hComboBox1, FALSE); // выбор глубины звука
				EnableWindow(hComboBox2, FALSE); // выбор частоты дискретизации
				EnableWindow(hComboBox3, FALSE); // выбор СТЕРЕО или МОНО
				EnableWindow(hComboBox5, FALSE); // выбор устройства записи

				// разрешаем работу потока записи
				RecStopFlag.store(false);

				// Запускаем поток:
				isRecThreadRunning.store(true);
				recThread = std::thread(RecordingThreadFunc);

				glVichMinMaxXY(NULL); // вычисляем границы текущего буфера для вывода его графика

			} // если поток записи ещё не запущен

		} // ID_REC
		break;

		// остановить запись
		case ID_STOP:
		{
			// Если поток не запущен – игнорируем
			if (isRecThreadRunning.load() == false)
			{
				MessageBox(hWnd, L"Поток записи ещё не запущен", \
					L"Ошибка", MB_OK | MB_ICONWARNING);
				break;
			}

			EnableWindow(hBut2, FALSE); // деактивируем кнопку СТОП

			// Сигналим потоку записи (через флаг), что нужно остановиться
			RecStopFlag.store(true);

		} // ID_STOP
		break;

		// предварительная визуализация (отображение вх потока без сохранения)
		case ID_CHECKBOX_NOSAVE:
		{
			// если уже запущено воспроизведение, то ничего не делаем
			if (isPlayThreadRunning.load() == true)
				break;

			// нажали на чекбокс
			if (HIWORD(wParam) == BN_CLICKED)
			{
				BOOL bChecked = SendMessage(hCheckBoxNoSave, BM_GETCHECK, 0, 0) == BST_CHECKED;

				// Если галочка ещё не стояла
				if (!bChecked)
				{
					if (Head_rec != NULL)
					{
						int MsgBoxResult = MessageBox(hWnd,\
							L"Предварительная визуализация удалит уже записанные данные.\n\nПродолжить?", \
							L"Внимание", MB_YESNO | MB_ICONWARNING);

						if (MsgBoxResult == IDNO)
							break; // не захотели удалять старую запись
					}

					// ставим галочку
					SendMessage(hCheckBoxNoSave, BM_SETCHECK, BST_CHECKED, 0);

					// только визуализация (без сохранения)
					RecNoSave.store(true);

					// имитируем нажатие кнопки записи
					SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(ID_REC, BN_CLICKED),\
						(LPARAM)hBut1);
				}

				// если галочка уже стояла
				else
				{
					// убираем галочку
					SendMessage(hCheckBoxNoSave, BM_SETCHECK, BST_UNCHECKED, 0);

					// Если поток записи ещё не запущен – сообщение об ошибке
					if (isRecThreadRunning.load() == false)
					{
						MessageBox(hWnd, L"Поток записи ещё не запущен",\
							L"Ошибка", MB_OK | MB_ICONWARNING);
						break;
					}

					// Сигналим потоку записи (через флаг), что нужно остановиться
					RecStopFlag.store(true);
				}

			} // if (HIWORD(wParam) == BN_CLICKED)
				
		} // ID_CHECKBOX_NOSAVE
		break;

		// начать воспр.
		case ID_START_PLAY:
		{

			if (Head_rec == NULL)
			{
				MessageBox(hWnd, L" Сначала запишите звук! ", L"Внимание!", MB_OK);
				break; // если ранее разговор не записан
			}

			if (Head_rec->next == NULL)
			{
				MessageBox(hWnd, L" Сначала запишите звук! ", L"Внимание!", MB_OK);
				break;// не воспр. если записано меньше 2х блоков данных
			}

			EnableWindow(hBut1, FALSE); // запись
			EnableWindow(hBut2, FALSE); // стоп запись
			EnableWindow(hBut3, FALSE); // воспр.

			// флаг разрешает воспроизведение
			PlayStopFlag.store(false);

			// Запускаем поток
			isPlayThreadRunning.store(true); // флаг того что поток запущен
			playThread = std::thread(PlayingThreadFunc);

			glVichMinMaxXY(NULL); // вычисляем границы текущего буфера для вывода его графика

		} // ID_START_PLAY
		break;

		case ID_STOP_PLAY:
		{
			PlayStopFlag.store(true);

		} //ID_STOP_PLAY
		break;

		case ID_PAUSE_PLAY:
		{
			if (isPlayThreadRunning.load() == true) // если воспроизводим звук
			{
				if (isNowPause == false) // когда ставим на паузу при проигрывании
				{
					isNowPause = true;
					SetWindowText(hButPPause, L"Продолжить"); //изменяем надпись кнопки "ВОСПР-ПАУЗА"
					waveOutPause(hPlay_device);
				}

				else // когда снимаем с паузы при проигрывании
				{
					isNowPause = false;
					SetWindowText(hButPPause, L"Пауза"); //изменяем надпись кнопки "ВОСПР-ПАУЗА"
					waveOutRestart(hPlay_device);
				}

			} // если воспроизводим звук

		} // ID_PAUSE_PLAY
		break;

		case ID_SOUND_CUT:
		{
			if (Head_rec == NULL)
			{
				MessageBox(hWnd, L" Сначала запишите или загрузите аудио! ", L"Внимание!", MB_OK | MB_ICONINFORMATION);
				break;
			}

			if (IsWindowEnabled(hBut2))
			{
				MessageBox(hWnd, L" Сначала остановите запись! ", L"Внимание!", MB_OK);
				break;
			}

			if (IsWindowEnabled(hBut5))
			{
				MessageBox(hWnd, L" Сначала остановите воспроизведение! ", L"Внимание!", MB_OK);
				break;
			}

			ULONG uCutStart, uCutFinish; // начало и конец выделения (диапазон от 0 до 10000)
			ULONG uBufKol; // сколько всего буферов данных записано в память

			uCutStart = (int)SendMessage(hTrBar1, TBM_GETPOS, 0, 0);

			// выделяем область на верхнем трекбаре
			SendMessage(hTrBar1, TBM_SETSEL, TRUE, MAKELONG(0, uCutStart));

			uCutFinish = (int)SendMessage(hTrBar2, TBM_GETPOS, 0, 0);

			// выделяем область на нижнем трекбаре
			double maxRange2 = SendMessage(hTrBar2, TBM_GETRANGEMAX, 0, 0); // узнаём макс зн. для трекбара 2
			SendMessage(hTrBar2, TBM_SETSEL, TRUE, MAKELONG(uCutFinish, maxRange2));

			if (uCutFinish < uCutStart + 1)
			{
				MessageBox(hWnd, L"Позиция в нижнем трекбаре должна быть больше чем в верхнем.", L"Внимание", \
					MB_OK | MB_ICONINFORMATION);

				// убираем выделение у обоих трекбаров
				SendMessage(hTrBar1, TBM_SETSEL, TRUE, MAKELPARAM(0, 0));
				SendMessage(hTrBar2, TBM_SETSEL, TRUE, MAKELPARAM(0, 0));

				SendMessage(hTrBar1, TBM_SETPOS, TRUE, 0); // трекбар 1 влево
				SendMessage(hTrBar2, TBM_SETPOS, TRUE, maxRange2); // трекбар 2 вправо

				break;
			}

			if (::RecBufferLenght > 0) // если звук уже записан
			{
				// считаем сколько всего буферов данных записано
				uBufKol = ::RecBufferLenght / ::BufferSize;

				// теперь в uCatStart и uCatFinish храним номера первого и последнего буферов
				uCutStart = (double)uCutStart * ((double)uBufKol / maxRange2);

				double dCutBuf1;

				dCutBuf1 = modf((double)uCutFinish * ((double)uBufKol / maxRange2), &dCutBuf1);
				if (dCutBuf1 > 0.5)
				{
					// если номер буфера больше *.5, то округляем в большую сторону
					uCutFinish = (double)uCutFinish * ((double)uBufKol / maxRange2) + 1;
				}
				else
				{
					// иначе округляем в меньшую сторону
					uCutFinish = (double)uCutFinish * ((double)uBufKol / maxRange2);
				}

				// повторно проверяем размер выделяемой области
				if (uCutFinish < uCutStart + 1)
				{
					MessageBox(hWnd, L"Слишком малый размер выделяемой области!", L"Внимание", MB_OK);

					// убираем выделение у обоих трекбаров
					SendMessage(hTrBar1, TBM_SETSEL, TRUE, MAKELPARAM(0, 0));
					SendMessage(hTrBar2, TBM_SETSEL, TRUE, MAKELPARAM(0, 0));

					SendMessage(hTrBar1, TBM_SETPOS, TRUE, 0); // трекбар 1 влево
					SendMessage(hTrBar2, TBM_SETPOS, TRUE, maxRange2); // трекбар 2 вправо

					break;
				}

				// размер выделяемой области > 0
				ListBoxAddStr(hList1, L"----------------------------");

				wcscpy(s1, L" Всего фрагментов - ");
				_itow(uBufKol, s, 10);
				wcscat(s1, s);
				ListBoxAddStr(hList1, s1);

				wcscpy(s1, L" Начальный фрагмент - ");
				_itow(uCutStart, s, 10);
				wcscat(s1, s);
				ListBoxAddStr(hList1, s1);

				wcscpy(s1, L" Количество скоп. фрагментов - ");
				_itow(uCutFinish - uCutStart, s, 10);
				wcscat(s1, s);
				ListBoxAddStr(hList1, s1);

				// uBufKol уже не нужен, храним в нём ответ от MsgBox()
				uBufKol = MessageBox(hWnd, L" Выполнить преобразование? ", L"Внимание", MB_YESNO | MB_ICONWARNING);
				if (uBufKol == IDYES)
				{
					// оставляем только выделенную середину
					CutSound(Head_rec, uCutStart, uCutFinish - uCutStart);

					ListBoxAddStr(hList1, L" Звуковые данные успешно обрезаны!");

					glVichMinMaxXY(Head_rec); // вычисляем границы буфера для вывода его графика
					PrepareSoundGraph(Head_rec, BufferSize, RecBufferLenght);

					InvalidateRect(hWnd, NULL, FALSE);  // заставляем окно перерисоватся
				}
				else
				{
					ListBoxAddStr(hList1, L" Пользователь отменил преобразованиe!");
				}

				ListBoxAddStr(hList1, L"----------------------------");

				// размер выделяемой области > 0

			} // если звук уже записан


			// убираем выделение у обоих трекбаров
			SendMessage(hTrBar1, TBM_SETSEL, TRUE, MAKELPARAM(0, 0));
			SendMessage(hTrBar2, TBM_SETSEL, TRUE, MAKELPARAM(0, 0));

			SendMessage(hTrBar1, TBM_SETPOS, TRUE, 0); // трекбар 1 влево
			SendMessage(hTrBar2, TBM_SETPOS, TRUE, maxRange2); // трекбар 2 вправо

		} // ID_SOUND_CUT
		break;

		///// меню /////////////////////////////////////////////////////////

		// вкл. быстрое отображение графика всего звука в режиме ожидания
		case ID_VIEW_GRAPH_QUICK_VISUALIZATION:
		{
			MENUITEMINFO mii = {};
			mii.cbSize = sizeof(MENUITEMINFO);
			mii.fMask = MIIM_STATE;

			// узнаём состояние данного пункта меню
			GetMenuItemInfo(GetMenu(hWnd), ID_VIEW_GRAPH_QUICK_VISUALIZATION, FALSE, &mii);

			// если граф звука mySoundGraph уже заполнен, то данный стиль разрешен
			if (mySoundGraph.Ready)
			{
				if (mii.fState & MFS_CHECKED) // если пункт уже был активирован, то ничего не делаем
				{
					if (mySoundGraph.Show == false) // на всякий случай проверим
					{
						mySoundGraph.Show = true;
						InvalidateRect(hWnd, NULL, FALSE);  // заставляем окно перерисоваться
					}
				}

				else // активируем данный пункт и перерисовываем главное окно
				{
					CheckMenuRadioItem(
						GetMenu(hWnd),
						ID_VIEW_GRAPH_QUICK_VISUALIZATION, // первый ID в группе
						ID_VIEW_GRAPH_DETAILED_VISUALIZATION, // последний ID в группе
						ID_VIEW_GRAPH_QUICK_VISUALIZATION, // отмечаем выбранный стиль
						MF_BYCOMMAND);

					mySoundGraph.Show = true;
					InvalidateRect(hWnd, NULL, FALSE);  // заставляем окно перерисоваться
				}

			} // mySoundGraph.Ready == true

			// если граф звука mySoundGraph не заполнен, то данный стиль запрещён
			else
			{
				// если пункт уже был деактивирован, то ничего не делаем
				if (mii.fState & MFS_UNCHECKED)
				{
					if (mySoundGraph.Show == true) // на всякий случай проверим
					{
						mySoundGraph.Show = false;
						InvalidateRect(hWnd, NULL, FALSE);  // заставляем окно перерисоваться
					}
				}

				else // принудительно активируем другой пункт, так как этот использовать невозможно
				{
					CheckMenuRadioItem(
						GetMenu(hWnd),
						ID_VIEW_GRAPH_QUICK_VISUALIZATION, // первый ID в группе
						ID_VIEW_GRAPH_DETAILED_VISUALIZATION, // последний ID в группе
						ID_VIEW_GRAPH_DETAILED_VISUALIZATION, // отмечаем другой стиль
						MF_BYCOMMAND);

					mySoundGraph.Show = false;
					InvalidateRect(hWnd, NULL, FALSE);  // заставляем окно перерисоватся
				}

			} // mySoundGraph.Ready == false

		}  // ID_VIEW_GRAPH_QUICK_VISUALIZATION
		break;
		
		// вкл. детальное отображение графика всего звука в режиме ожидания
		case ID_VIEW_GRAPH_DETAILED_VISUALIZATION:
		{
			MENUITEMINFO mii = {};
			mii.cbSize = sizeof(MENUITEMINFO);
			mii.fMask = MIIM_STATE;

			// узнаём состояние данного пункта меню
			GetMenuItemInfo(GetMenu(hWnd), ID_VIEW_GRAPH_DETAILED_VISUALIZATION, FALSE, &mii);

			// если данный пункт уже был активирован (выкл быстрая визуализация), то ничего не делаем
			if (mii.fState & MFS_CHECKED)
			{
				if (mySoundGraph.Show == true) // на всякий случай проверим
				{
					mySoundGraph.Show = false;
					InvalidateRect(hWnd, NULL, FALSE);  // заставляем окно перерисоваться
				}
			}				

			// если данный пункт ещё не был активирован, то активируем его и перерисовываем окно
			else 
			{
				CheckMenuRadioItem(
					GetMenu(hWnd),
					ID_VIEW_GRAPH_QUICK_VISUALIZATION, // первый ID в группе
					ID_VIEW_GRAPH_DETAILED_VISUALIZATION, // последний ID в группе
					ID_VIEW_GRAPH_DETAILED_VISUALIZATION, // отмечаем выбранный стиль
					MF_BYCOMMAND);

				mySoundGraph.Show = false;
				InvalidateRect(hWnd, NULL, FALSE);  // заставляем окно перерисоватся
			}

		} // ID_VIEW_GRAPH_DETAILED_VISUALIZATION
		break;

		// вкл/выкл отображение пиков/перегрузок (клиппинга) на графике всего звука в режиме ожидания
		case ID_VIEW_GRAPH_SHOW_OVERLOAD:
		{
			HMENU hMenu = GetMenu(hWnd); // Получаем главное меню окна
			
			// Узнаём состояние пункта меню
			UINT state = GetMenuState(hMenu, ID_VIEW_GRAPH_SHOW_OVERLOAD, MF_BYCOMMAND);

			if (state & MF_CHECKED) // убираем галочку
			{
				CheckMenuItem(hMenu, ID_VIEW_GRAPH_SHOW_OVERLOAD, MF_UNCHECKED);

				showPeaks = FALSE;
			}
			else // ставим галочку
			{
				CheckMenuItem(hMenu, ID_VIEW_GRAPH_SHOW_OVERLOAD, MF_CHECKED);

				showPeaks = TRUE;
			}

			InvalidateRect(hWnd, NULL, FALSE);  // заставляем окно перерисоватся

		} // ID_VIEW_GRAPH_SHOW_OVERLOAD
		break;

		// вкл/выкл детальное отображение звука во время записи/воспр.
		// (визуализация: всех точек звука, через одну)
		case ID_VIEW_DETAILED_SHOW:
		{
			HMENU hMenu = GetMenu(hWnd); // Получаем главное меню окна
			
			// Узнаём состояние пункта меню
			UINT state = GetMenuState(hMenu, ID_VIEW_DETAILED_SHOW, MF_BYCOMMAND);

			if (state & MF_CHECKED) // убираем галочку
			{
				CheckMenuItem(hMenu, ID_VIEW_DETAILED_SHOW, MF_UNCHECKED);
				DetailedShow = FALSE;

				// если выключили детальную визуализацию, пункт упр. перегрузкой деактивируем
				CheckMenuItem(hMenu, ID_VIEW_SHOW_OVERLOAD, MF_UNCHECKED);
				ShowOverload = FALSE;
				EnableMenuItem(hMenu, ID_VIEW_SHOW_OVERLOAD, MF_BYCOMMAND | MF_GRAYED);

				// если выключили детальную визуализацию, пункт VU meter деактивируем
				CheckMenuItem(hMenu, ID_VIEW_SHOW_VU_METER, MF_UNCHECKED);
				ShowVUMeter = FALSE;
				EnableMenuItem(hMenu, ID_VIEW_SHOW_VU_METER, MF_BYCOMMAND | MF_GRAYED);


			}
			else // ставим галочку
			{
				CheckMenuItem(hMenu, ID_VIEW_DETAILED_SHOW, MF_CHECKED);
				DetailedShow = TRUE;

				// если включили детальную визуализацию, пункт упр. перегрузкой активируем
				EnableMenuItem(hMenu, ID_VIEW_SHOW_OVERLOAD, MF_BYCOMMAND | MF_ENABLED);

				// если включили детальную визуализацию, пункт VU meter активируем
				EnableMenuItem(hMenu, ID_VIEW_SHOW_VU_METER, MF_BYCOMMAND | MF_ENABLED);

			}
		} // ID_VIEW_DETAILED_SHOW
		break;

		// отображать индикаторы перегрузки во время записи/воспр.
		case ID_VIEW_SHOW_OVERLOAD:
		{
			HMENU hMenu = GetMenu(hWnd); // Получаем главное меню окна
			// Изменяем состояние пункта меню
			UINT state = GetMenuState(hMenu, ID_VIEW_SHOW_OVERLOAD, MF_BYCOMMAND);

			if (state & MF_CHECKED) // убираем галочку
			{
				CheckMenuItem(hMenu, ID_VIEW_SHOW_OVERLOAD, MF_UNCHECKED);
				ShowOverload = FALSE;
			}
			else // ставим галочку
			{
				CheckMenuItem(hMenu, ID_VIEW_SHOW_OVERLOAD, MF_CHECKED);
				ShowOverload = TRUE;
			}
		} // ID_VIEW_SHOW_OVERLOAD
		break;

		// отображать VU meter во время записи/воспр.
		case ID_VIEW_SHOW_VU_METER:
		{
			HMENU hMenu = GetMenu(hWnd); // Получаем главное меню окна
			// Изменяем состояние пункта меню
			UINT state = GetMenuState(hMenu, ID_VIEW_SHOW_VU_METER, MF_BYCOMMAND);

			if (state & MF_CHECKED) // убираем галочку
			{
				CheckMenuItem(hMenu, ID_VIEW_SHOW_VU_METER, MF_UNCHECKED);
				ShowVUMeter = FALSE;
			}
			else // ставим галочку
			{
				CheckMenuItem(hMenu, ID_VIEW_SHOW_VU_METER, MF_CHECKED);
				ShowVUMeter = TRUE;
			}
		} // ID_VIEW_SHOW_VU_METER
		break;

		case ID_EDIT_CHANGEVOLUME:
		{
			if (Head_rec == NULL)
			{
				MessageBox(hWnd, L" Сначала запишите или загрузите аудио! ", L"Внимание!", MB_OK | MB_ICONINFORMATION);
				break;
			}

			if (IsWindowEnabled(hBut2))
			{
				MessageBox(hWnd, L" Сначала остановите запись! ", L"Внимание!", MB_OK);
				break;
			}

			if (IsWindowEnabled(hBut5))
			{
				MessageBox(hWnd, L" Сначала остановите воспроизведение! ", L"Внимание!", MB_OK);
				break;
			}

			// диалоговое окно изменения громкости (вызов функции изм. громкости там же)
			DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_FORM_VOLUMECHANGE), hWnd, VolumeDlgProc);

		} // ID_EDIT_CHANGEVOLUME
		break;

		case ID_EDIT_CLIPPING50:
		{
			if (Head_rec == NULL)
			{
				MessageBox(hWnd, L" Сначала запишите или загрузите аудио! ", L"Внимание!", MB_OK | MB_ICONINFORMATION);
				break;
			}

			if (IsWindowEnabled(hBut2))
			{
				MessageBox(hWnd, L" Сначала остановите запись! ", L"Внимание!", MB_OK);
				break;
			}

			if (IsWindowEnabled(hBut5))
			{
				MessageBox(hWnd, L" Сначала остановите воспроизведение! ", L"Внимание!", MB_OK);
				break;
			}

			if (MessageBox(hWnd, L"Вы уверены что хотите выполнить клиппирование?", L" ", \
				MB_ICONQUESTION | MB_YESNO) == IDYES)
			{

				//MessageBox(hWnd, L"можно клиппировать звук!", L"Ура!", MB_OK);
				EnableWindow(hBut1, FALSE); // дезактивируем кнопку начала записи
				EnableWindow(hBut3, FALSE); // дезактивируем кнопку начала воспроизведения

				Clipping(Head_rec, BufferSize, 0.5); // клиппирование на уровне 50%
				
				// обновить массив графа звука
				PrepareSoundGraph(Head_rec, BufferSize, RecBufferLenght);

				InvalidateRect(hWnd, NULL, FALSE);  // перерисовываем область OpenGL

				EnableWindow(hBut1, TRUE); // активируем кнопку начала записи
				EnableWindow(hBut3, TRUE); // активируем кнопку начала воспроизведения
			}

		} // ID_EDIT_CLIPPING50
		break;

		case ID_ABOUT_VERSION:
		{
			// строка для вывода в окно "О программе"
			wcscpy(s1, L"\n Sound Recorder by Sem ");
			wcscat(s1, MY_PROGRAM_VER);
			wcscat(s1, L" \n Unicode version \n Windows XP compatibility");
			wcscat(s1, L"\n\n Build date: ");

			// получаем дату компиляции в строку
			// Преобразование строки __DATE__ (char) в широкую строку (wchar_t)
			MultiByteToWideChar(CP_ACP, 0, __DATE__, -1, s, sizeof(s) / sizeof(s[0]));

			wcscat(s1, s);


			// пользователем настраиваиваемый MessageBox
			MSGBOXPARAMSW mbp; // для юникода
			mbp.cbSize = sizeof(MSGBOXPARAMSW);
			mbp.hwndOwner = hWnd;
			mbp.hInstance = GetModuleHandle(NULL);
			mbp.lpszText = s1;
			mbp.lpszCaption = L"About";
			mbp.dwStyle = MB_OK | MB_USERICON;
			mbp.dwLanguageId = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
			mbp.lpfnMsgBoxCallback = NULL;
			mbp.dwContextHelpId = 0;
			mbp.lpszIcon = MAKEINTRESOURCE(IDI_ICON_8BIT);
			MessageBoxIndirect(&mbp); // для юникода
		
		} // ID_ABOUT_VERSION
		break;

		case ID_ABOUT_OPENGL_INFO:
		{
			wcscpy(s1, L"Renderer: ");
			const char* renderer = (const char*)glGetString(GL_RENDERER);
			swprintf(s, 250, L"%hs", renderer);
			wcscat(s1, s);
			
			wcscat(s1, L"\nVendor: ");
			const char* vendor = (const char*)glGetString(GL_VENDOR);
			swprintf(s, 250, L"%hs", vendor);
			wcscat(s1, s);

			wcscat(s1, L"\nVersion: ");
			const char* version = (const char*)glGetString(GL_VERSION);
			swprintf(s, 250, L"%hs", version);
			wcscat(s1, s);

			MessageBox(hWnd, s1, L"OpenGL Info", MB_OK);

		} // ID_ABOUT_OPENGL_INFO
		break;

		case ID_FILE_EXIT:
		{
			PostMessage(hWnd, WM_CLOSE, 0, 0); // закрываем приложение

		}
		break;

		case ID_FILE_WAVE_OPEN:
		{
			WCHAR WaveOpenFileName[256] = { 0 };

			if (!IsWindowEnabled(hBut2)) // если в данный момент не идёт запись звука
			{
				if (!IsWindowEnabled(hBut5)) // если звук не воспроизводится
				{
					OPENFILENAME ofn; // переменная для настройки окна диалога

					// Initialize OPENFILENAME
					ZeroMemory(&ofn, sizeof(ofn));
					ofn.lStructSize = sizeof(ofn);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = WaveOpenFileName;// сюда сохраняем имя файла
					ofn.nMaxFile = 255; // размер буфера под имя файла
					ofn.lpstrFilter = L"All files (*.*)\0*.*\0WAVE PCM Files(*.wav)\0*.wav;*wave\0\0";
					ofn.nFilterIndex = 2; // выбран второй фильтр по умолчанию (wave...)
					ofn.lpstrFileTitle = NULL; // имя по умолчанию
					ofn.nMaxFileTitle = 0; // длина имени по умолчанию
					ofn.lpstrInitialDir = NULL; // стартовая папка для поиска нужного ф-ла
					ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST; // файл должен существовать

					// Если получили имя файла то открываем его
					if (GetOpenFileName(&ofn))
					{
						// файл открываем
						if (WaveRead(WaveOpenFileName))
							MessageBox(hWnd, L" Файл успешно открыт!", L"Success", MB_OK);
						else MessageBox(hWnd, L"Не удалось открыть файл", L"Error", MB_OK | MB_ICONERROR);
					}
				}
				else MessageBox(hWnd, L"Сначала остановите воспроизведение!", L"Ошибка", MB_OK);
			}
			else MessageBox(hWnd, L"Сначала остановите запись звука!", L"Ошибка", MB_OK);
		
		} // ID_FILE_WAVE_OPEN
		break;

		case ID_FILE_WAVE_SAVE_AS:
		{
			if (!IsWindowEnabled(hBut2)) // если в данный момент не идёт запись звука
			{
				// храним имена записанных файлов
				WCHAR WaveSaveFileName[256] = { 0 };

				OPENFILENAME ofs;

				// Initialize OPENFILENAME
				ZeroMemory(&ofs, sizeof(ofs));
				ofs.lStructSize = sizeof(ofs);
				ofs.hwndOwner = hWnd;
				ofs.hInstance = NULL;
				ofs.lpstrFilter = L"All files (*.*)\0*.*\0WAVE PCM Files(*.wav)\0*.wav;*wave\0\0";
				ofs.nFilterIndex = 2;// по умолчанию выбран второй фильтр
				ofs.lpstrFile = WaveSaveFileName;
				ofs.nMaxFile = 255; // размер буфера под имя файла
				ofs.lpstrFileTitle = NULL; // сюда сохраняем только имя файла, без пути
				ofs.nMaxFileTitle = NULL; // размер буфера под предыдущую переменную
				ofs.lpstrInitialDir = NULL; // стартовая папка для поиска нужного ф-ла
				// запрос при создании нового, не показывать RO файлы, запрос при перезаписи файла
				ofs.Flags = OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT;
				ofs.lpstrDefExt = L"wav"; // по умолчанию расширение
				ofs.lpstrTitle = L" Сохранить как...";

				// если успешно получили имя файла то выполняем функцию сохранения
				if (GetSaveFileName(&ofs))
				{ // файл существует и его можно открыть
					ListBoxAddStr(hList1, L"Путь сохранения файла: ");
					//MessageBox(hWnd,"Получено имя файла", "Успех", MB_OK);
					wcscpy(s1, L" ");
					wcsncat(s1, WaveSaveFileName, 100);
					wcscat(s1, L"..");
					ListBoxAddStr(hList1, s1);

					// пытаемся сохранить данные
					if (WaveSave(WaveSaveFileName, Head_rec, ::RecBufferLenght, BufferSize, struct_play_data) != TRUE)
					{
						if(Head_rec == NULL)
							MessageBox(hWnd, L" Нет данных для сохранения! ", L"Внимание", MB_OK | MB_ICONINFORMATION);
						else
							MessageBox(hWnd, L" Проблемы при сохранении файла! ", L"Ошибка", MB_OK | MB_ICONERROR);
					}
					else
					{
						wcscpy(s1, L"Путь сохранения:\n ");
						wcsncat(s1, WaveSaveFileName, 100);
						wcscat(s1, L"..");
						MessageBox(hWnd, s1, L"Файл успешно сохранён", MB_OK);
					}

				} // файл существует и его можно открыть
				else
				{
					ListBoxAddStr(hList1, L"Не удалось получить имя файла для сохранения!");
					MessageBox(hWnd, L"Не удалось получить имя файла!", L"Ошибка", MB_OK | MB_ICONERROR);
				}
			} // не идёт запись звука

			else // кнопка СТОП (записи) активна, значит идёт запись, запрещено сохранять
			{
				MessageBox(hWnd, L"Сначала остановите запись!", L"Ошибка", MB_OK);
			}

		} // ID_FILE_WAVE_SAVE_AS
		break;

		}// WM_COMMAND - конец обработки статус бара и кнопок
		break;

	// действия при нажатии крестика закрытия окна
	case WM_CLOSE:
	{
		// Если поток записи запущен и включена запись (а не просто визуализация)
		if (isRecThreadRunning.load() == true && RecNoSave.load() == false)
		{
			if (MessageBoxW(hWnd, L"Остановить запись и выйти?", L"Подтверждение",\
				MB_YESNO | MB_ICONQUESTION) == IDYES)
			{
				DestroyWindow(hWnd); // идём дальше в WM_DESTROY
			}
			// если выбрали нет - ничего не делаем
		}
		// Если поток записи не запущен
		else
			DestroyWindow(hWnd); // идём дальше в WM_DESTROY
	
	} // WM_CLOSE
	break;

	// действия после закрытия этого окна
	case WM_DESTROY:
	{
		// Если поток записи запущен
		if (isRecThreadRunning.load() == true)
		{
			// Сигналим потоку записи (через флаг), что нужно остановиться
			RecStopFlag.store(true);

			// Ждём, пока поток записи корректно завершится
			if (recThread.joinable())
			{
				recThread.join();
			}

			isRecThreadRunning.store(false); // помечаем, что поток уже остановлен
		} // Если поток записи запущен

		// если поток воспроизведения запущен
		if (isPlayThreadRunning.load() == true)
		{
			// Сигналим потоку воспроизведения (через флаг), что нужно остановиться
			PlayStopFlag.store(true); // флаг остановки

			// Ждём, пока поток воспроизведения корректно завершится
			if (playThread.joinable())
			{
				playThread.join();
			}

			isPlayThreadRunning.store(false); // помечаем, что поток уже остановлен
		} // если поток воспроизведения запущен

		// чистим GDI
		if (WhiteColorBrush) { DeleteObject(WhiteColorBrush); WhiteColorBrush = NULL; }

		PostQuitMessage(0);

	} // WM_DESTROY
	break;

	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);

	} // switch (msg)
	
	return 0;

}



// Version 3.x - Unicode version   
// используем версию WinMain для Юникода - wWinMain (с соответствующими параметрами)

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrevInst, PWSTR pCmdLine, int nWinMode)
{

	MSG msg;
	WNDCLASS wcl; // класс для нашего главного окна

	// инициализируем стили курсора
	hCursorHand = LoadCursor(NULL, IDC_HAND);
	hCursorArrow = LoadCursor(NULL, IDC_ARROW);
	hCursorWait = LoadCursor(NULL, IDC_WAIT);

	// цвет фона GroupBox
	WhiteColorBrush = CreateSolidBrush(RGB(255, 255, 255));

	// начальные настройки функций для работы с DPI (масштабирование элементов окна)
	InitDpi();

	::hInst = hInst;

	wcl.hInstance = hInst;
	wcl.lpszClassName = L"Sound_win_Class";
	wcl.lpfnWndProc = WindowFunc;
	wcl.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS; // у каждого окна свой контекст вывода
	wcl.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON_8BIT));
	wcl.hCursor = NULL; // будем менять стиль курсора позже при движении мышки внутри окна
	wcl.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
	wcl.cbClsExtra = 0;
	wcl.cbWndExtra = 0;
	wcl.hbrBackground = WhiteColorBrush; // цвет фона окна 
	
	if (!RegisterClass(&wcl)) 
	{
		MessageBox(NULL, L" Ошибка при создании класса окна! ", L"RegisterClass()", MB_OK | MB_ICONERROR);
		return 1;
	}

	// наше главное окно рисуем по центру экрана
	RECT rcScreen;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &rcScreen, 0); // Получаем размеры экрана
	// Вычисляем центр
	int posX = rcScreen.left + ((rcScreen.right - rcScreen.left) - wW) / 2;
	int posY = rcScreen.top + ((rcScreen.bottom - rcScreen.top) - wH) / 2;


	hWnd = CreateWindowEx(0, L"Sound_win_Class", L"Sound Recorder",
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 
		posX, 
		posY,
		wW,
		wH,
		HWND_DESKTOP, NULL,
		hInst, NULL);

	if (!hWnd) 
	{ 
		MessageBox(NULL, L" Ошибка при создании окна программы! ", L"CreateWindow()", MB_OK | MB_ICONERROR);
		return 1;
	}

	ShowWindow(hWnd, nWinMode);
	UpdateWindow(hWnd);

	// (КОД ДЛЯ WINE) 
	// перерисовываем фоны hGroupBox, чтобы не было артефактов
	// Получение текущего размера окна
	RECT rect;
	//GetClientRect(hWnd, &rect);
	// перерисовываем элементы окна и фоны hGroupBox
	//RedrawWindowElements(rect.right - rect.left, rect.bottom - rect.top);
	// обновляем все элементы окна после перерисовки фона
	//RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
	// (КОД ДЛЯ WINE)


	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;

}

		





