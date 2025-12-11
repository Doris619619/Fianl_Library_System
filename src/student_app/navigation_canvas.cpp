#include <seatui/student/navigation_canvas.hpp>
#include <QPainter>
#include <QLinearGradient>//to paint gradient background
#include <QFont>//for word font setting
#include <QImage>
#include <QtMath>

NavigationCanvas::NavigationCanvas(QWidget* parent) : QWidget(parent) {
    setMinimumSize(680, 440);//set a minimum size for the canvas
    setAutoFillBackground(false);//we paint the background ourself
}

// using SSAA which is a good way to improve visual quality
void NavigationCanvas::setSuperSample(bool on){
    if (useSSAA != on) { useSSAA = on; update(); }
}

// Resize event: update layout, when you strech the window,keep it the same
void NavigationCanvas::resizeEvent(QResizeEvent*){
    updateLayout(width(), height());
    msaaBuffer = QImage(width()*2, height()*2, QImage::Format_RGBA8888);
    //using transparent background for buffer
    msaaBuffer.fill(Qt::transparent);
}

//update layout according to the new width and height
void NavigationCanvas::updateLayout(int W, int H){
    lay.W = W; lay.H = H;
    lay.margin = int(0.06 * qMin(W, H));
    lay.rectInner = QRect(lay.margin, lay.margin, W - 2*lay.margin, H - 2*lay.margin);

    // four shelf centers A/B/C/D
    const int n = 4;
    const int colW = lay.rectInner.width() / n;
    lay.shelfCenters.clear();
    for (int k = 0; k < n; ++k) {
        const int x = lay.rectInner.left() + colW * k + colW / 2;
        const int y = lay.rectInner.top() + int(0.12 * lay.rectInner.height());
        lay.shelfCenters.push_back(QPoint(x, y));
    }

    // START：right bottom, aligned to grid
    const int cell = m_cell;
    const int rx = snapDn(lay.rectInner.right(), cell)  - m_borderCells * cell - 2*cell;
    const int ry = snapDn(lay.rectInner.bottom(), cell) - m_borderCells * cell - 2*cell;
    lay.startPt = QPoint(rx, ry);
}

void NavigationCanvas::drawBackgroundGrid(QPainter& p){
    // background gradient
    QLinearGradient g(0, 0, 0, lay.H);
    g.setColorAt(0.0, QColor(10,14,24));
    g.setColorAt(1.0, QColor(16,19,27));//；lighter color
    p.fillRect(QRect(0,0,lay.W,lay.H), g);
// let the angle be smoother
    p.setRenderHint(QPainter::Antialiasing, true);

    // Main area //
    // outter boundary
    QPen outer(QColor(60, 80, 100, 180));
    outer.setWidthF(2.0);
    p.setPen(outer);
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(lay.rectInner.adjusted(0.5, 0.5, -0.5, -0.5), 12, 12);

    // inner boundary
    QPen inner(QColor(36,45,60, 160));
    inner.setWidthF(1.0);
    p.setPen(inner);
    p.drawRoundedRect(lay.rectInner.adjusted(6, 6, -6, -6), 10, 10);

    //  m_cell, every four seats set a line//
    const int cell = m_cell;
    // based on lay.rectInner 
    const int Lg = snapUp(lay.rectInner.left(),  cell);
    const int Tg = snapUp(lay.rectInner.top(),   cell);
    const int Rg = snapDn(lay.rectInner.right(), cell);
    const int Bg = snapDn(lay.rectInner.bottom(),cell);

    QPen minorPen(QColor(70, 86,108, 70));  minorPen.setWidth(1);
    QPen majorPen(QColor(100,120,140,120)); majorPen.setWidth(1);

    for (int x = Lg, idx = 0; x <= Rg; x += cell, ++idx) {
        p.setPen(idx % 4 == 0 ? majorPen : minorPen);
        p.drawLine(x, Tg, x, Bg);
    }
    for (int y = Tg, idy = 0; y <= Bg; y += cell, ++idy) {
        p.setPen(idy % 4 == 0 ? majorPen : minorPen);
        p.drawLine(Lg, y, Rg, y);
    }
}


void NavigationCanvas::drawShelvesLabels(QPainter& p){
    const int cell = m_cell;                          // size of one grid cell
    const int badgeW = 4 * cell;                      // label badge width = 4 cells
    const int badgeH = 2 * cell;                     

    // Vertical position: one grid cell below the top inner grid (avoid seats)
    const int Tg = snapUp(lay.rectInner.top(), cell); // top aligned to grid
    const int y  = Tg + cell;                         // shift down by 1 cell

    const QString labels[4] = {u8"A",u8"B",u8"C",u8"D"}; // label texts
    const int n = 4;                                 // number of columns
    const int colW = lay.rectInner.width() / n;      // width of each column segment

    for (int i = 0; i < n; ++i) {
        // compute center of column, align to grid, clamp inside boundary
        const int cx = lay.rectInner.left() + colW * i + colW / 2; // center of each section
        int left = snapDn(cx - badgeW/2, cell);                    // align left to grid

        // clamp inside left/right boundary with 2px margin
        left = qMax(left, lay.rectInner.left() + 2);
        if (left + badgeW > lay.rectInner.right() - 2)
            left = lay.rectInner.right() - 2 - badgeW;

        QRect r(left, y, badgeW, badgeH);            // label rectangle

        // background panel
        p.setPen(QPen(QColor(55,67,85), 1));         
        p.setBrush(QColor(20,30,48,220));          
        p.drawRoundedRect(r, 10, 10);                

        // text drawing
        p.setPen(QColor(224,229,236));               // light text color
        QFont f = p.font(); f.setBold(true);
        f.setPointSizeF(qMax(10.0, lay.H*0.022));    // auto-scale with height
        p.setFont(f);
        p.drawText(r, Qt::AlignCenter, labels[i]);   // centered label text
    }
}


void NavigationCanvas::drawSeats(QPainter& p){
    const int cell = m_cell;                         // grid cell size

    // Grid-aligned usable area; reserve top cells for labels
    const int Lg = snapUp(lay.rectInner.left(),  cell);   
    const int Tg = snapUp(lay.rectInner.top(),   cell);  
    const int Rg = snapDn(lay.rectInner.right(), cell);    
    const int Bg = snapDn(lay.rectInner.bottom(),cell);    

    const int L_area = Lg;                          // usable left boundary
    const int T_area = Tg + m_topReserve * cell;    // skip top reserved rows
    const int R_area = Rg;                          // usable right boundary
    const int B_area = Bg;                          // usable bottom boundary

    // seat size and stepping distance (including aisle spacing)
    const int seatW = m_seatCols * cell;            // seat width = 4 cells
    const int seatH = m_seatRows * cell;            
    const int stepX = seatW + m_aisleXCells * cell; // horizontal step including aisle
    const int stepY = seatH + m_aisleYCells * cell; 

    // 1) available width/height within boundaries
    const int availableWidth = R_area - L_area - 2;     // subtract 2px margin
    const int availableHeight = B_area - T_area - 2;    // subtract 2px margin

    // 2) max rows/cols that can fit inside area
    const int maxCols = (availableWidth + m_aisleXCells * cell) / (stepX);
    const int maxRows = (availableHeight + m_aisleYCells * cell) / (stepY);

    // ensure at least one row/column exists
    const int numCols = qMax(1, maxCols);
    const int numRows = qMax(1, maxRows);

    // 3) calculate total content width/height of seats
    const int totalContentWidth = numCols * stepX - m_aisleXCells * cell;
    const int totalContentHeight = numRows * stepY - m_aisleYCells * cell;

    // 4) compute start position; horizontally fixed (2px), vertical centered
    const int startX = L_area + 2;                          // 2px left padding
    const int startY = T_area + (availableHeight - totalContentHeight) / 2; // vertical center

    // 5) generate X and Y positions for seats
    QVector<int> xs, ys;
    for (int col = 0; col < numCols; ++col) {
        xs.push_back(startX + col * stepX);     // x positions for each column
    }
    for (int row = 0; row < numRows; ++row) {
        ys.push_back(startY + row * stepY);     // y positions for each row
    }

    // remove leftmost & rightmost seat columns if more than 2 columns
    if (xs.size() > 2) {
        xs.removeFirst();                       // delete leftmost column
        xs.removeLast();                        // delete rightmost column
    }

    if (xs.isEmpty() || ys.isEmpty()) return;   // no seats to draw

    // seat drawing style
    QPen seatPen(QColor(130, 160, 180, 110));   // subtle seat border
    seatPen.setWidthF(1.2);

    // draw all seats
    for (int y : ys) {
        for (int x : xs) {
            QRectF seatRect(x + m_seatGap, y + m_seatGap,
                            seatW - 2*m_seatGap, seatH - 2*m_seatGap); // inner rectangle

            p.setPen(Qt::NoPen);                        // no border for fill
            p.setBrush(QColor(26,34,48,220));           // seat fill color
            p.drawRoundedRect(seatRect, m_seatRadius, m_seatRadius); // rounded seat

            p.setBrush(Qt::NoBrush);                    // outline only
            p.setPen(seatPen);                          // seat border
            p.drawRoundedRect(seatRect, m_seatRadius, m_seatRadius);
        }
    }
}


void NavigationCanvas::drawStartMark(QPainter& p){
    p.setBrush(QColor(56,189,248));                  // Set fill color for the start mark
    p.setPen(Qt::NoPen);                             // No outline for the circle
    const int r = int(0.012 * qMin(lay.W, lay.H));   // Radius based on canvas size
    p.drawEllipse(lay.startPt, r, r);                

    p.setPen(QColor(200,225,240));                   // Set text color
    QFont f = p.font(); f.setBold(true);             
    f.setPointSizeF(qMax(9.0, lay.H*0.018));         // Set font size based on height
    p.setFont(f);                                    // Apply font setting
    p.drawText(lay.startPt + QPoint(12, -6), "START"); // Draw "START" text near point
}

void NavigationCanvas::drawScene(QPainter& p, const QSize&){
    drawBackgroundGrid(p);                           // Draw main grid (with thicker line every 4 cells)
    drawShelvesLabels(p);                            // Draw A/B/C/D shelf labels
    drawSeats(p);                                   
    drawStartMark(p);                              
}

void NavigationCanvas::paintEvent(QPaintEvent*){
    if (useSSAA) {                                   // If supersampling anti-aliasing enabled
        if (msaaBuffer.size() != QSize(width()*2, height()*2))
            msaaBuffer = QImage(width()*2, height()*2, QImage::Format_RGBA8888);  // Recreate buffer if size mismatch
        msaaBuffer.fill(Qt::transparent);            // Clear buffer (transparent)

        QPainter pm(&msaaBuffer);                   
        pm.setRenderHint(QPainter::Antialiasing, true);  
        pm.scale(2.0, 2.0);                          
        drawScene(pm, QSize(width(), height()));     // Draw everything into the buffer
        pm.end();                                    

        QPainter p(this);                            // Painter for widget
        p.setRenderHint(QPainter::SmoothPixmapTransform, true); // Smooth scaling
        p.drawImage(rect(), msaaBuffer);             // Draw MSAA image to screen
    } else {
        QPainter p(this);                            // Painter for widget
        p.setRenderHint(QPainter::Antialiasing, true); 
        drawScene(p, size());                 
    }
}
