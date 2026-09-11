import QtQuick

// Direct port of app.py DrawingArea — single logic, no redesign
Item {
    id: root
    property var model
    property int countVisible: 7
    property string borderColor: "#b4befe"
    property int borderWidth: 2
    property int idleBorderWidth: 2
    property string idleBorderColor: "#585b70"
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
        // keep centered on resize, but never cancel an in-flight step
        if (anim.running) return
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
            // single rate for position and scale: highlight no longer leads
            // the slide. f clamped both sides: first frame after idle is a
            // full step (no frozen start), lag spikes at most double it.
            let f = Math.min(Math.max(anim.frameTime, 1/60), 1/30) * 60
            let k = 1 - Math.pow(1 - 0.18, f)
            let dx = targetX - contentX
            let ds = targetSelection - visualSelection
            let doneX = Math.abs(dx) < 0.5
            let doneS = Math.abs(ds) < 0.01
            if (doneX) contentX = targetX; else contentX += dx * k
            if (doneS) visualSelection = targetSelection; else visualSelection += ds * k
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
            if (root.count === 0) return
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
            if (root.count === 0) { dragging=false; return }
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
            property int center: Math.round(root.visualSelection)
            property int vIdx: root.isSmallCount ? index : center - root.visibleRange + index
            property int realIdx: root.isSmallCount ? index : ((vIdx % root.count)+root.count)%root.count
            // these come from model
            property string wallpaperPath: root.model ? root.model.get_path_at(realIdx) : ""
            property string wallpaperName: root.model ? root.model.get_name_at(realIdx) : ""
            property string thumbnailPath: root.model ? root.model.get_thumb_at(realIdx) : ""

            property real dist: Math.abs(vIdx - root.visualSelection)
            property real progress: Math.max(0, 1 - dist)
            property real scaledW: root.tileWidth * (1 + (root.horizontalScale - 1.0) * progress)
            property real scaledH: root.panelHeight * (1 + (root.verticalScale - 1.0) * progress)
            property real shearOff: root.shear * scaledH
            property real baseX: vIdx * root.step - root.contentX
            property real compX: baseX + root.extraWidth/2
                                 * Math.max(-1, Math.min(1, vIdx - root.visualSelection))
            // Original: left = x + tile/2 - scaledW/2 - shear/2
            x: compX + root.tileWidth/2 - scaledW/2 - shearOff/2
            y: (root.height - scaledH)/2
            width: scaledW + shearOff
            height: scaledH
            z: isSelected ? 100 : 50 - Math.floor(dist)
            visible: !(x > root.width + root.margin || x + width < -root.margin)
            property bool isSelected: root.isSmallCount ? realIdx === root.selectedIndex : vIdx === Math.round(root.visualSelection)

            // Parallelogram clip: sheared box, image counter-sheared so the
            // picture itself stays upright (matches the original clip+paint).
            Item {
                id: tileClip
                width: scaledW
                height: scaledH
                clip: true
                transform: Matrix4x4 {
                    matrix: Qt.matrix4x4(1, -root.shear, 0, shearOff,
                                         0, 1, 0, 0,
                                         0, 0, 1, 0,
                                         0, 0, 0, 1)
                }

                Image {
                    source: thumbnailPath
                    width: scaledW + shearOff
                    height: scaledH
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                    retainWhileLoading: true
                    cache: true
                    // fixed cap: scale-dependent sourceSize would re-decode every frame
                    sourceSize.height: Math.round(root.panelHeight * root.verticalScale)
                    transform: Matrix4x4 {
                        matrix: Qt.matrix4x4(1, root.shear, 0, -shearOff,
                                             0, 1, 0, 0,
                                             0, 0, 1, 0,
                                             0, 0, 0, 1)
                    }
                }

                Rectangle {
                    anchors.fill: parent
                    color: "black"
                    opacity: 0.16
                    visible: !isSelected
                }
            }

            // Border is stroked outside the clip, like the original reset_clip
            Rectangle {
                width: scaledW
                height: scaledH
                color: "transparent"
                border.width: isSelected ? root.borderWidth : root.idleBorderWidth
                border.color: isSelected ? root.borderColor : root.idleBorderColor
                visible: border.width > 0
                transform: Matrix4x4 {
                    matrix: Qt.matrix4x4(1, -root.shear, 0, shearOff,
                                         0, 1, 0, 0,
                                         0, 0, 1, 0,
                                         0, 0, 0, 1)
                }
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

    // Decode preloader: warms QML's pixmap cache for every wallpaper, so a
    // window rebind mid-slide swaps to an already-decoded image in the same
    // frame instead of showing stale tiles for 1-3 frames. Invisible items
    // still decode; they never paint.
    Repeater {
        model: root.count
        delegate: Image {
            required property int index
            source: root.model ? root.model.get_thumb_at(index) : ""
            visible: false
            asynchronous: true
            cache: true
        }
    }
}
