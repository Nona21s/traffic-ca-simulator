# Traffic CA Simulator

Mô phỏng Cellular Automaton cho một nút giao thông bốn hướng, viết bằng C++17. Ứng dụng gồm lõi mô phỏng, giao diện Qt Widgets, chương trình dòng lệnh và benchmark CSV. Các làn xe cập nhật bằng double buffering; luồng sinh xe dựa trên seed nên chạy tuần tự và OpenMP cho cùng kết quả.

## Mô hình phiên bản đầu

- Mỗi hướng có một làn đi thẳng vào nút giao; xe đi qua nút và rời mô hình, chưa mô phỏng rẽ hoặc người đi bộ.
- Đèn dùng hai pha cố định, không xung đột: Bắc/Nam xanh rồi Đông/Tây xanh. Thời lượng mỗi pha cấu hình được; chưa có pha vàng.
- Mỗi tick, xe tiến tối đa một ô nếu ô phía trước trống. Xe đầu hàng chỉ rời nút khi hướng của xe đang xanh.
- `arrival_rate` là xác suất xuất hiện một xe ở cuối mỗi làn trong mỗi tick, nếu ô đó trống. `initial_density` là xác suất có xe ở từng ô lúc khởi tạo.
- `average_waiting_time` tính trên các xe đã rời mô hình; `average_queue_length` là số xe trung bình còn trong mô hình sau mỗi tick.

## Yêu cầu

- Windows 10/11, Visual Studio Community với workload **Desktop development with C++** (MSVC).
- CMake 3.21 trở lên.
- OpenMP do MSVC cung cấp; CMake tìm và liên kết qua `OpenMP::OpenMP_CXX`.
- Qt 6.2 trở lên, cài riêng bằng Qt Online Installer, chọn Qt Widgets và bộ kit MSVC tương ứng với Visual Studio.
- Python 3 chỉ cần nếu muốn dùng script benchmark; `matplotlib` chỉ cần nếu muốn vẽ biểu đồ.

Không có mã nguồn/dependency nào được tải tự động. Nếu Qt chưa được cài hoặc CMake chưa tìm thấy Qt, cấu hình sẽ dừng với hướng dẫn đặt `CMAKE_PREFIX_PATH`.

## Build trên Windows

Mở **Developer PowerShell for VS** để MSVC có sẵn trên `PATH`. Thay `msvc20xx_64` trong đường dẫn bên dưới theo kit đã cài:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\msvc20xx_64"
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Với Visual Studio generator, cấu hình là multi-config; các executable nằm trong `build\Release`. Mở thư mục này bằng VS Code để sửa mã, hoặc mở thư mục dự án trong Visual Studio.

## Chạy trực tiếp trong Visual Studio

1. Chọn **File → Open → Folder...** và mở thư mục gốc có `CMakeLists.txt`.
2. Với Qt cài ở đường dẫn mặc định, cấu hình CMake lần đầu bằng:

   ```powershell
   cmake -S . -B out\build\x64-Debug -DCMAKE_PREFIX_PATH="C:\Qt\6.12.0\msvc2022_64"
   ```

   Có thể chạy lệnh trong **Developer PowerShell for VS**. Visual Studio dùng cấu hình thư mục `out\build\x64-Debug`.
3. Trong Visual Studio, chờ CMake tải lại; nếu thấy thanh **CMake Generation Failed**, chọn **Generate** sau khi đã cấu hình Qt ở bước trên.
4. Trên thanh công cụ, chọn `x64-Debug`, mở danh sách startup item (nếu đang chọn project khác hoặc danh sách trống) và chọn **`traffic_ca_gui.exe`**.
5. Nhấn nút tam giác xanh hoặc **Ctrl+F5** để chạy không cần debugger; nhấn **F5** để chạy có debugger.

Các target CLI, benchmark và tests cũng hiện trong danh sách startup items. Chọn `traffic_ca.exe` để chạy CLI; test có thể chạy từ **Test → Run All Tests**.

## Chạy

GUI:

```powershell
$env:PATH = "C:\Qt\6.x.x\msvc20xx_64\bin;$env:PATH"
.\build\Release\traffic_ca_gui.exe
```

GUI cho phép chọn kịch bản, chỉnh số tick, chiều dài làn, xác suất xe đến, mật độ ban đầu, seed và thời lượng đèn; có Start/Pause, Step, Reset và tùy chọn OpenMP. Khi thay đổi tham số, mô phỏng sẽ tạm dừng; Start hoặc Step áp dụng cấu hình mới từ đầu. Reset khởi tạo lại theo giá trị đang hiển thị.

Để đóng gói GUI trên Windows, chạy `windeployqt .\build\Release\traffic_ca_gui.exe` từ Qt kit tương ứng; lệnh này sao chép Qt runtime và plugin nền tảng cần thiết cạnh executable.

CLI:

```powershell
.\build\Release\traffic_ca.exe --steps 2000 --seed 123 --arrival-rate 0.3 --parallel
.\build\Release\traffic_ca.exe --scenario .\config\scenarios.json
```

CLI mặc định tải kịch bản đầu tiên từ `config\scenarios.json`. Kết quả in ra throughput, thời gian chờ trung bình, độ dài hàng trung bình, xe còn lại và state hash.

## Benchmark

```powershell
.\scripts\run_benchmark.ps1
python .\scripts\plot.py .\results\benchmark.csv
```

Runner gọi cả chế độ tuần tự và OpenMP, lặp nhiều lần rồi ghi `results\benchmark.csv`. Có thể truyền `-BuildDirectory`, `-OutputPath`, `-Steps` và `-Repetitions` cho script PowerShell. Cài matplotlib tùy chọn bằng `python -m pip install matplotlib` nếu cần biểu đồ.

## Cấu trúc

- `src/`: kiểu dữ liệu, cấu hình, RNG xác định, grid, mô hình pha đèn, engine, metrics, CLI, GUI và benchmark.
- `config/scenarios.json`: các kịch bản mẫu được GUI/CLI đọc.
- `tests/test_all.cpp`: kiểm thử lõi, pha đèn, tín hiệu đỏ/xanh, seed và tính tương đương OpenMP.
- `scripts/`: benchmark runner cho PowerShell và script vẽ CSV tùy chọn.
- `results/`: nơi lưu kết quả benchmark.
