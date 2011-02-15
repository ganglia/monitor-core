(function ($) {
    $.fn.gangZoom = function (options) {
        var opts = $.extend({}, $.fn.gangZoom.defaults, options);

        if (! $.fn.gangZoom.styleAdded && opts.defaultStyle) {
            $("<style type='text/css'>" + opts.defaultStyle + "</style>").appendTo("head");
            $.fn.gangZoom.styleAdded = true;
        }

        return this.each(function() {
            var self = this;
            self.img = $(self);
            self.mouseDown = false;

            self.getTimes = function () {
                var timeSpan = opts.endTime - opts.startTime;
                var graphWidth = self.img.width() - (opts.paddingLeft + opts.paddingRight);
                var secondsPerPixel = timeSpan / graphWidth;
                var selWidth = self.float.width();
                var selStart = self.float.position().left - self.img.offset().left;
                var selStartTime = opts.startTime + ((selStart - opts.paddingLeft) * secondsPerPixel);
                var selEndTime = selWidth * secondsPerPixel + selStartTime;

                return { startTime: selStartTime, endTime: selEndTime };
            }

            self.cancel = function () {
                self.mouseDown = false;
                var box = self.getTimes();
                opts.cancel(box.startTime, box.endTime);
            };

            self.go = function () {
                self.mouseDown = false;
                var box = self.getTimes();
                opts.done(box.startTime, box.endTime);
            };

            $(document.body).mouseup(function (event) {
                if (self.mouseDown) {
                    if (event.target == self.img || event.target == self.float[0]) {
                        self.go();
                    } else {
                        self.cancel();
                    }
                }
            })
            .keyup(function (event) {
                if (event.keyCode == 27 && self.mouseDown) {
                    self.cancel();
                }
            });

            self.img.mousedown(function (event) {
                event.preventDefault();
                self.shouldStopClick = false;
                var evt = event;

                setTimeout(function () {
                    evt.stopPropagation();

                    self.shouldStopClick = true;
                    self.mouseDown = true;
                    console.log("setting mousedown: " + self.mouseDown);

                    $("#" + opts.floatId).remove();

                    var clickX = evt.pageX;
                    var clickY = evt.pageY;

                    self.startX = clickX;

                    var float = $("<div id='" + opts.floatId + "'>")
                        .css({ position: "absolute", left: clickX, top: clickY - 5, zIndex: 1000 })
                        .width(10)
                        .height(25)
                        .mousemove(function() { return true; })
                        .mouseup(function() { return true; })
                        .keyup(function() { return true; })
                        .appendTo(document.body);

                    self.float = float;

                }, opts.clickTimeout);
            })
            .mousemove(function (evt) {
                if (self.mouseDown && self.float) {
                    var clickX = evt.pageX;
                    if (self.startX > clickX) {
                        self.float.css({ left: clickX });
                    }

                    self.float.width(Math.abs(clickX - self.startX));
                }
            })
            .mouseup(function (evt) {
                console.log("MOUSE UP");
                console.log(self.mouseDown);
                if (self.mouseDown) {
                    self.go();
                }
            })
            .click(function (event) {
                console.log("click");
                if (self.shouldStopClick) {
                    event.preventDefault();
                }
            });
        });
    }

    $.fn.gangZoom.defaults = {
        clickTimeout: 500,
        floatId: 'gangZoomFloater',
        defaultStyle: "#gangZoomFloater { border: 1px solid black; background: white; opacity: 0.7 }",
        startTime: ((new Date()).getTime() / 1000) - 3600,
        endTime: (new Date()).getTime() / 1000,
        paddingLeft: 20,
        paddingRight: 20,
        done: function (startTime, endTime) {},
        cancel: function (startTime, endTime) {}
    }

    $.fn.gangZoom.styleAdded = false;
})(jQuery);
