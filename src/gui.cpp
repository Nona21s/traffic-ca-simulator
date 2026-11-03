#include "engine.hpp"
#include "scenario_io.hpp"
#include "scenario_path.hpp"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QPaintEvent>
#include <QPainter>
#include <QPushButton>
#include <QSpinBox>
#include <QSizePolicy>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

class IntersectionView final : public QWidget {
public:
    explicit IntersectionView(QWidget* parent = nullptr)
        : QWidget(parent), grid_(24, 0.0, 0) {
        setMinimumSize(660, 570);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

    void set_state(const traffic::Grid& grid, traffic::SignalPhase phase) {
        grid_ = grid;
        phase_ = phase;
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(rect(), QColor(236, 240, 232));

        const qreal center_x = width() / 2.0;
        const qreal center_y = height() / 2.0;
        const qreal road_width = 142.0;
        const qreal half_road = road_width / 2.0;
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(65, 70, 76));
        painter.drawRect(QRectF(center_x - half_road, 0, road_width, height()));
        painter.drawRect(QRectF(0, center_y - half_road, width(), road_width));

        QPen lane_marking(QColor(235, 225, 170), 2.0, Qt::DashLine);
        painter.setPen(lane_marking);
        painter.drawLine(QPointF(center_x, 0), QPointF(center_x, center_y - half_road));
        painter.drawLine(QPointF(center_x, center_y + half_road), QPointF(center_x, height()));
        painter.drawLine(QPointF(0, center_y), QPointF(center_x - half_road, center_y));
        painter.drawLine(QPointF(center_x + half_road, center_y), QPointF(width(), center_y));

        draw_vehicles(painter, center_x, center_y, half_road);
        draw_signals(painter, center_x, center_y, half_road);

        painter.setPen(QColor(32, 38, 42));
        painter.drawText(QRectF(0, 6, width(), 24), Qt::AlignHCenter | Qt::AlignVCenter,
                         QStringLiteral("BẮC"));
        painter.drawText(QRectF(0, height() - 30, width(), 24),
                         Qt::AlignHCenter | Qt::AlignVCenter, QStringLiteral("NAM"));
        painter.drawText(QRectF(8, center_y - 12, 72, 24),
                         Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("TÂY"));
        painter.drawText(QRectF(width() - 80, center_y - 12, 72, 24),
                         Qt::AlignRight | Qt::AlignVCenter, QStringLiteral("ĐÔNG"));
    }

private:
    void draw_vehicles(QPainter& painter, qreal center_x, qreal center_y, qreal half_road) {
        const qreal available_length = std::min(center_x - half_road, center_y - half_road);
        const qreal cell_spacing = std::min(18.0, available_length / grid_.length());
        const qreal vehicle_width = std::max(3.0, cell_spacing * 0.82);
        const qreal vehicle_length = std::max(4.0, cell_spacing * 0.65);
        painter.setPen(QPen(QColor(235, 245, 255), 1.0));
        painter.setBrush(QColor(49, 142, 207));

        for (traffic::Approach approach : traffic::all_approaches) {
            const traffic::Lane& lane = grid_.lane(approach);
            for (std::size_t cell = 0; cell < lane.size(); ++cell) {
                if (!lane[cell].has_value()) {
                    continue;
                }
                qreal x = center_x;
                qreal y = center_y;
                qreal width = vehicle_width;
                qreal height = vehicle_length;
                switch (approach) {
                case traffic::Approach::North:
                    x = center_x - 32.0;
                    y = center_y - half_road - (static_cast<qreal>(cell) + 0.5) * cell_spacing;
                    break;
                case traffic::Approach::South:
                    x = center_x + 17.0;
                    y = center_y + half_road + (static_cast<qreal>(cell) + 0.5) * cell_spacing;
                    break;
                case traffic::Approach::East:
                    x = center_x + half_road + (static_cast<qreal>(cell) + 0.5) * cell_spacing;
                    y = center_y - 32.0;
                    width = vehicle_length;
                    height = vehicle_width;
                    break;
                case traffic::Approach::West:
                    x = center_x - half_road - (static_cast<qreal>(cell) + 0.5) * cell_spacing;
                    y = center_y + 17.0;
                    width = vehicle_length;
                    height = vehicle_width;
                    break;
                }
                painter.drawRoundedRect(QRectF(x, y, width, height), 3.0, 3.0);
            }
        }
    }

    void draw_signals(QPainter& painter, qreal center_x, qreal center_y, qreal half_road) {
        const bool ns_green = phase_ == traffic::SignalPhase::NorthSouth;
        const QColor green(45, 205, 90);
        const QColor red(226, 66, 66);
        painter.setPen(QPen(QColor(25, 28, 30), 2.0));
        const auto light = [&painter](qreal x, qreal y, const QColor& color) {
            painter.setBrush(color);
            painter.drawEllipse(QPointF(x, y), 9.0, 9.0);
        };
        light(center_x - half_road - 17.0, center_y - half_road - 17.0, ns_green ? green : red);
        light(center_x + half_road + 17.0, center_y + half_road + 17.0, ns_green ? green : red);
        light(center_x + half_road + 17.0, center_y - half_road - 17.0, ns_green ? red : green);
        light(center_x - half_road - 17.0, center_y + half_road + 17.0, ns_green ? red : green);
    }

    traffic::Grid grid_;
    traffic::SignalPhase phase_ = traffic::SignalPhase::NorthSouth;
};

class TrafficWindow final : public QMainWindow {
public:
    explicit TrafficWindow(std::vector<traffic::NamedScenario> scenarios)
        : scenarios_(std::move(scenarios)), engine_(nullptr) {
        setWindowTitle(QStringLiteral("Mô phỏng giao thông CA"));
        resize(1100, 680);

        auto* central = new QWidget(this);
        auto* root = new QHBoxLayout(central);
        auto* controls = new QWidget(central);
        controls->setMaximumWidth(330);
        auto* panel = new QVBoxLayout(controls);
        auto* form = new QFormLayout();

        scenario_box_ = new QComboBox(controls);
        for (const auto& scenario : scenarios_) {
            scenario_box_->addItem(scenario.name);
        }
        form->addRow(QStringLiteral("Kịch bản"), scenario_box_);

        steps_box_ = new QSpinBox(controls);
        steps_box_->setRange(1, 10000000);
        form->addRow(QStringLiteral("Số bước mô phỏng"), steps_box_);

        length_box_ = new QSpinBox(controls);
        length_box_->setRange(2, 500);
        form->addRow(QStringLiteral("Số ô mỗi hướng"), length_box_);

        arrival_box_ = new QDoubleSpinBox(controls);
        arrival_box_->setRange(0.0, 1.0);
        arrival_box_->setDecimals(2);
        arrival_box_->setSingleStep(0.05);
        form->addRow(QStringLiteral("Xác suất xe đến"), arrival_box_);

        density_box_ = new QDoubleSpinBox(controls);
        density_box_->setRange(0.0, 1.0);
        density_box_->setDecimals(2);
        density_box_->setSingleStep(0.05);
        form->addRow(QStringLiteral("Mật độ xe ban đầu"), density_box_);

        seed_box_ = new QSpinBox(controls);
        seed_box_->setRange(0, 2147483647);
        form->addRow(QStringLiteral("Hạt giống ngẫu nhiên"), seed_box_);

        ns_green_box_ = new QSpinBox(controls);
        ns_green_box_->setRange(1, 1000000);
        form->addRow(QStringLiteral("Đèn xanh Bắc/Nam (bước)"), ns_green_box_);

        ew_green_box_ = new QSpinBox(controls);
        ew_green_box_->setRange(1, 1000000);
        form->addRow(QStringLiteral("Đèn xanh Đông/Tây (bước)"), ew_green_box_);

        parallel_box_ = new QCheckBox(QStringLiteral("Cập nhật song song bằng OpenMP"), controls);
        form->addRow(parallel_box_);
        panel->addLayout(form);

        auto* buttons = new QHBoxLayout();
        run_button_ = new QPushButton(QStringLiteral("Chạy"), controls);
        auto* step_button = new QPushButton(QStringLiteral("Bước tiếp"), controls);
        auto* reset_button = new QPushButton(QStringLiteral("Đặt lại"), controls);
        buttons->addWidget(run_button_);
        buttons->addWidget(step_button);
        buttons->addWidget(reset_button);
        panel->addLayout(buttons);

        status_label_ = new QLabel(controls);
        status_label_->setWordWrap(true);
        panel->addWidget(status_label_);
        panel->addStretch();

        view_ = new IntersectionView(central);
        root->addWidget(controls);
        root->addWidget(view_, 1);
        setCentralWidget(central);

        timer_.setInterval(100);
        connect(&timer_, &QTimer::timeout, this, [this]() { advance_once(); });
        connect(run_button_, &QPushButton::clicked, this, [this]() { toggle_running(); });
        connect(step_button, &QPushButton::clicked, this, [this]() { advance_once(); });
        connect(reset_button, &QPushButton::clicked, this, [this]() { reset_simulation(); });
        connect(scenario_box_, qOverload<int>(&QComboBox::currentIndexChanged),
                this, [this](int) { apply_scenario(); });
        apply_scenario();
        connect(steps_box_, qOverload<int>(&QSpinBox::valueChanged),
                this, [this](int) { mark_dirty(); });
        connect(length_box_, qOverload<int>(&QSpinBox::valueChanged),
                this, [this](int) { mark_dirty(); });
        connect(arrival_box_, qOverload<double>(&QDoubleSpinBox::valueChanged),
                this, [this](double) { mark_dirty(); });
        connect(density_box_, qOverload<double>(&QDoubleSpinBox::valueChanged),
                this, [this](double) { mark_dirty(); });
        connect(seed_box_, qOverload<int>(&QSpinBox::valueChanged),
                this, [this](int) { mark_dirty(); });
        connect(ns_green_box_, qOverload<int>(&QSpinBox::valueChanged),
                this, [this](int) { mark_dirty(); });
        connect(ew_green_box_, qOverload<int>(&QSpinBox::valueChanged),
                this, [this](int) { mark_dirty(); });
        connect(parallel_box_, &QCheckBox::toggled, this, [this](bool) { mark_dirty(); });
    }

private:
    void apply_scenario() {
        const int index = scenario_box_->currentIndex();
        if (index < 0 || static_cast<std::size_t>(index) >= scenarios_.size()) {
            return;
        }
        const auto& config = scenarios_[static_cast<std::size_t>(index)].config;
        steps_box_->setValue(static_cast<int>(config.steps));
        length_box_->setValue(static_cast<int>(config.approach_length));
        arrival_box_->setValue(config.arrival_rate);
        density_box_->setValue(config.initial_density);
        seed_box_->setValue(static_cast<int>(config.seed % 2147483648ULL));
        ns_green_box_->setValue(static_cast<int>(config.north_south_green_ticks));
        ew_green_box_->setValue(static_cast<int>(config.east_west_green_ticks));
        reset_simulation();
    }

    traffic::SimulationConfig selected_config() const {
        traffic::SimulationConfig config =
            scenarios_[static_cast<std::size_t>(scenario_box_->currentIndex())].config;
        config.steps = static_cast<std::uint64_t>(steps_box_->value());
        config.approach_length = static_cast<std::uint32_t>(length_box_->value());
        config.arrival_rate = arrival_box_->value();
        config.initial_density = density_box_->value();
        config.seed = static_cast<std::uint64_t>(seed_box_->value());
        config.north_south_green_ticks = static_cast<std::uint32_t>(ns_green_box_->value());
        config.east_west_green_ticks = static_cast<std::uint32_t>(ew_green_box_->value());
        config.use_openmp = parallel_box_->isChecked();
        return config;
    }

    void reset_simulation() {
        timer_.stop();
        run_button_->setText(QStringLiteral("Chạy"));
        try {
            engine_ = std::make_unique<traffic::SimulationEngine>(selected_config());
            dirty_ = false;
            refresh_view();
        } catch (const std::exception& error) {
            QMessageBox::critical(this, QStringLiteral("Lỗi cấu hình mô phỏng"),
                                  QString::fromLocal8Bit(error.what()));
        }
    }

    void mark_dirty() {
        dirty_ = true;
        timer_.stop();
        run_button_->setText(QStringLiteral("Chạy"));
    }

    void toggle_running() {
        if (dirty_) {
            reset_simulation();
        }
        if (!engine_) {
            reset_simulation();
        }
        if (timer_.isActive()) {
            timer_.stop();
            run_button_->setText(QStringLiteral("Chạy"));
        } else if (engine_ && engine_->tick() < engine_->config().steps) {
            timer_.start();
            run_button_->setText(QStringLiteral("Tạm dừng"));
        }
    }

    void advance_once() {
        if (dirty_) {
            reset_simulation();
        }
        if (!engine_ || engine_->tick() >= engine_->config().steps) {
            timer_.stop();
            run_button_->setText(QStringLiteral("Chạy"));
            return;
        }
        engine_->step();
        refresh_view();
        if (engine_->tick() >= engine_->config().steps) {
            timer_.stop();
            run_button_->setText(QStringLiteral("Chạy"));
        }
    }

    void refresh_view() {
        if (!engine_) {
            return;
        }
        view_->set_state(engine_->grid(), engine_->current_phase());
        const auto& metrics = engine_->metrics();
        status_label_->setText(
            QStringLiteral("Bước mô phỏng: %1 / %2\nĐèn xanh: %3\nXe còn lại: %4\n"
                           "Xe đã qua nút: %5\nThời gian chờ trung bình: %6 bước\n"
                           "Số xe xếp hàng trung bình: %7")
                .arg(engine_->tick())
                .arg(engine_->config().steps)
                .arg(engine_->current_phase() == traffic::SignalPhase::NorthSouth
                         ? QStringLiteral("Bắc / Nam")
                         : QStringLiteral("Đông / Tây"))
                .arg(engine_->grid().vehicle_count())
                .arg(metrics.departed())
                .arg(metrics.average_waiting_time(), 0, 'f', 2)
                .arg(metrics.average_queue_length(), 0, 'f', 2));
    }

    std::vector<traffic::NamedScenario> scenarios_;
    std::unique_ptr<traffic::SimulationEngine> engine_;
    QTimer timer_;
    QComboBox* scenario_box_ = nullptr;
    QSpinBox* steps_box_ = nullptr;
    QSpinBox* length_box_ = nullptr;
    QDoubleSpinBox* arrival_box_ = nullptr;
    QDoubleSpinBox* density_box_ = nullptr;
    QSpinBox* seed_box_ = nullptr;
    QSpinBox* ns_green_box_ = nullptr;
    QSpinBox* ew_green_box_ = nullptr;
    QCheckBox* parallel_box_ = nullptr;
    QPushButton* run_button_ = nullptr;
    QLabel* status_label_ = nullptr;
    IntersectionView* view_ = nullptr;
    bool dirty_ = false;
};

} // namespace

int main(int argc, char* argv[]) {
    QApplication application(argc, argv);
    try {
        auto scenarios = traffic::load_scenarios(traffic::resolve_scenario_path());
        TrafficWindow window(std::move(scenarios));
        window.show();
        return application.exec();
    } catch (const std::exception& error) {
        QMessageBox::critical(nullptr, QStringLiteral("Lỗi khởi động"),
                              QString::fromLocal8Bit(error.what()));
        return 1;
    }
}
