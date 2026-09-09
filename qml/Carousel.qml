import QtQuick

// Direct port of app.py DrawingArea — single logic, no redesign
Item {
    id: root
    property var model
    property int countVisible: 7
    property string borderColor: "#b4befe"
    property int borderWidth: 4
    property real panelHeight: 500
    property real shear: 0.3
    property real spacing: 4.0
    property real horizontalScale: 1.6
    property real verticalScale: 1.1

    signal wallpaperClicked(int idx)
    signal applyRequested(int idx)

    property real visualSelection: 0
    property real targetSelection: 0
    property real contentX: 0
    property real targetX: 0

    property real tileWidth: Math.max(1, width / countVisible - 10)
    property real step: tileWidth + spacing
    property real extraWidth: tileWidth * (horizontalScale - 1.0)
    property real margin: tileWidth * 0.25
    property int count: 0
    property int visualCenter: Math.round(visualSelection)

    Connections {
        target: root.model
        function onCountChanged() { root.count = root.model ? root.model.count() : 0 }
    }
    onModelChanged: root.count = model ? model.count() : 0

    function setSelected(idx) {
        if (count === 0) return
        if (isSmallCount) {
            idx = Math.max(0, Math.min(count - 1, idx))
            targetSelection = idx
            ensureVisible(idx)
            startAnim()
            return
        }
        idx = ((idx % count) + count) % count
        let cur = ((Math.round(targetSelection) % count) + count) % count
        let delta = idx - cur
        if (delta > count/2) delta -= count
        if (delta < -count/2) delta += count
        targetSelection += delta
        ensureVisible(targetSelection)
        startAnim()
    }
    function next() {
        if (isSmallCount) { setSelected(Math.min(count - 1, Math.round(targetSelection) + 1)); return }
        setSelected(((Math.round(targetSelection)+1)%count+count)%count)
    }
    function prev() {
        if (isSmallCount) { setSelected(Math.max(0, Math.round(targetSelection) - 1)); return }
        setSelected(((Math.round(targetSelection)-1)%count+count)%count)
    }
    function jumpForward() {
        if (isSmallCount) { setSelected(Math.min(count - 1, Math.round(targetSelection) + countVisible)); return }
        let c=countVisible; let t=Math.round(targetSelection)+c; setSelected(((t%count)+count)%count)
    }
    function jumpBack() {
        if (isSmallCount) { setSelected(Math.max(0, Math.round(targetSelection) - countVisible)); return }
        let c=countVisible; let t=Math.round(targetSelection)-c; setSelected(((t%count)+count)%count)
    }

    function ensureVisible(idx) {
        let center = idx * step + tileWidth / 2
        let viewportCenter = width / 2
        let x = center - viewportCenter
        if (isSmallCount) {
            let minX = tileWidth/2 - width/2
            let maxX = (count - 1) * step + tileWidth/2 - width/2
            x = Math.max(minX, Math.min(maxX, x))
        }
        targetX = x
        startAnim()
    }
    function startAnim() { anim.restart() }

    function setSelectedImmediate(idx) {
        if (count===0) return
        if (isSmallCount) idx = Math.max(0, Math.min(count - 1, idx))
        else idx = ((idx%count)+count)%count
        visualSelection = idx
        targetSelection = idx
        contentX = idx * step + tileWidth/2 - width/2
        targetX = contentX
        anim.running = false
    }
    Component.onCompleted: {
        root.count = model ? model.count() : 0
    }
    onWidthChanged: {
        // keep centered on resize
        let idx = Math.round(visualSelection)
        if (count>0 && width>100) {
            contentX = idx*step + tileWidth/2 - width/2
            targetX = contentX
        }
    }

    FrameAnimation {
        id: anim
        running: false
        onTriggered: {
            let dx = targetX - contentX
            let ds = targetSelection - visualSelection
            let doneX = Math.abs(dx) < 0.5
            let doneS = Math.abs(ds) < 0.01
            if (doneX) contentX = targetX; else contentX += dx * 0.15
            if (doneS) visualSelection = targetSelection; else visualSelection += ds * 0.22
            if (!isSmallCount && count>0 && Math.abs(visualSelection) >= count) {
                let off = Math.floor(visualSelection / count) * count
                visualSelection -= off; targetSelection -= off
                contentX -= off*step; targetX -= off*step
            }
            if (doneX && doneS) running=false
        }
    }
    readonly property int selectedIndex: ((Math.round(visualSelection)% (count||1))+(count||1))%(count||1)

    WheelHandler {
        onWheel: (e) => {
            let d = Math.abs(e.angleDelta.y) >= Math.abs(e.angleDelta.x) ? e.angleDelta.y : e.angleDelta.x
            let nx = targetX - d * 0.8
            if (isSmallCount) {
                let minX = tileWidth/2 - width/2
                let maxX = (count - 1) * step + tileWidth/2 - width/2
                nx = Math.max(minX, Math.min(maxX, nx))
                let idx = Math.round((nx + width/2 - tileWidth/2) / step)
                idx = Math.max(0, Math.min(count - 1, idx))
                targetSelection = idx
                visualSelection = idx
            } else {
                let idx = Math.round((nx + width/2 - tileWidth/2) / step)
                idx = ((idx % count) + count) % count
                let cur = ((Math.round(targetSelection) % count) + count) % count
                let delta = idx - cur
                if (delta > count/2) delta -= count
                if (delta < -count/2) delta += count
                targetSelection += delta
                visualSelection = targetSelection
            }
            targetX = nx
            startAnim()
        }
    }
    MouseArea {
        id: dragArea
        anchors.fill: parent
        property real startX
        property real startContentX
        property bool dragging:false
        onPressed: (m)=>{ startX=m.x; startContentX=targetX; dragging=false }
        onPositionChanged: (m)=>{
            let off=m.x-startX
            if(!dragging && Math.abs(off)>8) dragging=true
            if(dragging){
                let nx = startContentX-off
                if (isSmallCount) {
                    let minX = tileWidth/2 - width/2
                    let maxX = (count - 1) * step + tileWidth/2 - width/2
                    nx = Math.max(minX, Math.min(maxX, nx))
                }
                targetX=nx; contentX=targetX; anim.running=false
            }
        }
        onReleased: (m)=>{
            if(dragging){
                if (isSmallCount) {
                    let idx = Math.round((targetX + width/2 - tileWidth/2) / step)
                    idx = Math.max(0, Math.min(count - 1, idx))
                    targetSelection = idx; visualSelection = idx
                    ensureVisible(idx)
                } else {
                    let idx = Math.round((targetX + width/2 - tileWidth/2) / step)
                    idx = ((idx % count) + count) % count
                    let cur = ((Math.round(targetSelection) % count) + count) % count
                    let delta = idx - cur
                    if (delta > count/2) delta -= count
                    if (delta < -count/2) delta += count
                    targetSelection += delta
                    visualSelection = targetSelection
                    ensureVisible(targetSelection)
                    anim.restart()
                }
            }
            dragging=false
        }
    }

    property int visibleRange: Math.floor(countVisible/2)+2
    property bool isSmallCount: root.count > 0 && root.count <= root.visibleRange*2+1
    Repeater {
        model: root.count>0 ? (root.isSmallCount ? root.count : root.visibleRange*2+1) : 0
        delegate: Item {
            required property int index
            property int center: Math.floor(root.visualSelection)
            property int vIdx: root.isSmallCount ? index : center - root.visibleRange + index
            property int realIdx: root.isSmallCount ? index : ((vIdx % root.count)+root.count)%root.count
            // these come from model
            property string wallpaperPath: root.model ? root.model.get_path_at(realIdx) : ""
            property string wallpaperName: root.model ? root.model.get_name_at(realIdx) : ""
            property string thumbnailPath: root.model ? root.model.get_thumb_at(realIdx) : ""

            property real dist: Math.abs(vIdx - root.visualSelection)
            property real progress: Math.max(0, 1 - dist)
            property real scaledW: root.tileWidth * (1 + 0.6 * progress)
            property real scaledH: root.panelHeight * (1 + 0.1 * progress)
            property real shearOff: root.shear * scaledH
            property real baseX: vIdx * root.step - root.contentX
            property real compX: {
                if (vIdx < root.visualCenter) return baseX - root.extraWidth/2
                if (vIdx > root.visualCenter) return baseX + root.extraWidth/2
                return baseX
            }
            // Original: left = x + tile/2 - scaledW/2 - shear/2
            x: compX + root.tileWidth/2 - scaledW/2 - shearOff/2
            y: (root.height - scaledH)/2
            width: scaledW + shearOff
            height: scaledH
            z: isSelected ? 100 : 50 - Math.floor(dist)
            visible: !(x > root.width + root.margin || x + width < -root.margin)
            property bool isSelected: root.isSmallCount ? realIdx === root.selectedIndex : vIdx === Math.round(root.visualSelection)

            Image {
                id: hiddenImg
                source: thumbnailPath
                visible: false
                asynchronous: true
                retainWhileLoading: true
                cache: true
                onStatusChanged: if (status===Image.Ready) canvas.requestPaint()
            }

            Canvas {
                id: canvas
                anchors.fill: parent
                // trigger repaint when selection animates
                property real _v: root.visualSelection
                property real _c: root.contentX
                on_VChanged: requestPaint()
                on_CChanged: requestPaint()
                onPaint: {
                    var ctx = getContext("2d")
                    ctx.reset()
                    // clip parallelogram
                    ctx.beginPath()
                    ctx.moveTo(shearOff, 0)
                    ctx.lineTo(width, 0)
                    ctx.lineTo(width - shearOff, height)
                    ctx.lineTo(0, height)
                    ctx.closePath()
                    ctx.clip()

                    // cover image — draw retained buffer too, never blank
                    if (hiddenImg.status === Image.Ready || hiddenImg.implicitWidth > 0) {
                        var pw = hiddenImg.sourceSize.width || hiddenImg.implicitWidth
                        var ph = hiddenImg.sourceSize.height || hiddenImg.implicitHeight
                        if (pw>0 && ph>0) {
                            var scale = Math.max(width / pw, height / ph)
                            var dw = pw * scale
                            var dh = ph * scale
                            var ox = (width - dw)/2
                            var oy = (height - dh)/2
                            // draw
                            ctx.drawImage(hiddenImg, ox, oy, dw, dh)
                            // dim unselected — original 0.16 black
                            if (!isSelected) {
                                ctx.fillStyle = "rgba(0,0,0,0.16)"
                                ctx.fillRect(0,0,width,height)
                            }
                        }
                    } else {
                        ctx.clearRect(0,0,width,height)
                    }

                    // border for selected — width from config
                    if (isSelected) {
                        ctx.strokeStyle = root.borderColor
                        ctx.lineWidth = root.borderWidth
                        ctx.beginPath()
                        ctx.moveTo(shearOff, 0)
                        ctx.lineTo(width, 0)
                        ctx.lineTo(width - shearOff, height)
                        ctx.lineTo(0, height)
                        ctx.closePath()
                        ctx.stroke()
                    }
                }
                Component.onCompleted: requestPaint()
                Connections { target: hiddenImg; function onStatusChanged(){ canvas.requestPaint() } }
                Connections { target: root; function onVisualSelectionChanged(){ canvas.requestPaint() } }
                Connections { target: root; function onContentXChanged(){ canvas.requestPaint() } }
            }

            // Click handling
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (isSelected) root.applyRequested(realIdx)
                    else root.setSelected(realIdx)
                    root.wallpaperClicked(realIdx)
                }
            }
        }
    }
}
