angular.module('npact')

  .controller('npactGraphContainerCtrl', function($scope, $element, $window, $log,
                                           npactConstants, Utils, GraphConfig, Evt,
                                           GraphingCalculator, headerSpecCalc) {
    'use strict';

    //The mouse event responders
    var onPan = function(opts) {
          $log.log('Pan event:', opts);
          var offset = Math.floor(opts.newStartBase - opts.oldStartBase);
          GraphConfig.offset += offset;
          $scope.$apply();
        },
        onZoom = function(opts) {
          $log.log('Zoom event:', opts);
          var zoomOpts = angular.extend({}, GraphConfig, opts);
          //updates `offset`, and `basesPerGraph`
          angular.extend(GraphConfig, GraphingCalculator.zoom(zoomOpts));
          $scope.$apply();
        };

    //The baseOpts are the graph options that are the same for every graph
    var baseOpts = angular.extend({ width: $element.width(),
                                    onPan: onPan, onZoom: onZoom },
                                  npactConstants.graphSpecDefaults);
    this.graphOptions = function(idx) {
      // This function builds the specific options for a graph, the
      // options object is lazily generated when needed
      var range = $scope.graphSpecs[idx],
          opts = angular.extend({}, baseOpts, range);
      opts.m = GraphingCalculator.chart(opts);
      opts.xaxis = GraphingCalculator.xaxis(opts);
      return opts;
    };

    var repartition = function() {
          $scope.graphSpecs = GraphConfig.partition();
          $log.log('Partitioned into', $scope.graphSpecs.length, 'rows.');
          updateVisibility();
          rebuild();
    },
        draw = function() { $scope.$broadcast(Evt.DRAW); },
        redraw = function() { $scope.$broadcast(Evt.REDRAW); },
        rebuild = function() { $scope.$broadcast(Evt.REBUILD); };


    // watch the environment for changes we care about
    $scope.$watchCollection(function() {
      return [ GraphConfig.length, GraphConfig.basesPerGraph,
               GraphConfig.offset, GraphConfig.startBase, GraphConfig.endBase];
    }, repartition);

    $scope.$watch(GraphConfig.profileTitle, function(title) {
      //A profileTitle change indicates different nucleotides: rebuild
      baseOpts.axisTitle = title;
      rebuild();
    });

    $scope.$watch(GraphConfig.activeTracks, function(val) {
      //Find headers and headerY
      angular.extend(baseOpts, headerSpecCalc(val));
      redraw();
    }, true);

    $scope.$watch(
      function() { return GraphConfig.colorBlindFriendly; },
      function(val) {
        baseOpts.colors = val ?
          npactConstants.colorBlindLineColors : npactConstants.lineColors;
        redraw();
    });


    /***  Scrolling and Graph Visibility management ***/
    var $win = angular.element($window),
        winHeight = $win.height(),
        borderHeight = 1,
        graphBoxHeight = npactConstants.graphSpecDefaults.height + borderHeight,
        slack = 50, // how many pixels outside of viewport to render
        topOffset = $element.offset().top,
        topIdx = 0, bottomIdx = 0,
        updateVisibility = function() {
          var scrollDist = $window.scrollY - topOffset - slack;
          topIdx = Math.floor(scrollDist / graphBoxHeight);
          bottomIdx = topIdx + Math.ceil((winHeight) / graphBoxHeight);
        },
        onScroll = function() {
          updateVisibility();
          draw();
        },
        onResize = function() {
          winHeight = $win.height();
          if($element.width() !== baseOpts.width) {
            topOffset = $element.offset().top;
            baseOpts.width = $element.width();
            updateVisibility();
            redraw();
          }
          else {
            //If the width didn't change then its the same as scrolling
            onScroll();
          }
        };
    $scope.graphHeight = npactConstants.graphSpecDefaults.height;
    this.visible = function(idx) {
      return idx >= topIdx && idx <= bottomIdx;
    };
    $win.on('resize', onResize);
    $win.on('scroll', onScroll);
    $scope.$on('$destroy', function() {
      $win.off('resize', onResize);
      $win.off('scroll', onScroll);
    });
  })
  .directive('npactGraphContainer', function(STATIC_BASE_URL) {
    return {
      restrict: 'A',
      scope: {},
      templateUrl: STATIC_BASE_URL + 'js/graphs/graph-container.html',
      controller: 'npactGraphContainerCtrl as ctrl'
    };
  })

  .directive('npactGraph', function (Grapher, Evt, GraphingCalculator, $log, $timeout) {
    'use strict';
    return {
      restrict: 'A',
      require: '^npactGraphContainer',
      link: function($scope, $element, $attrs, ctrl) {
        var g = null,
            visible = ctrl.visible,
            idx = $attrs.idx,
            el = $element[0],
            // redraw gets set for all graphs once (e.g. a new track
            // triggers broadcasts redraw), but only gets cleared as
            // the currently visible ones are drawn
            redraw = false,
            drawTimer = null,
            draw = function() {
              if(!visible(idx)) { drawTimer = null; return; }
              var opts = ctrl.graphOptions(idx);
              (g || (g = new Grapher(el, opts)))
                .redraw(opts)
              //redraw doesn't return a promise at the moment, but that should return
              // .then(function() {
                //   redraw = false;
                //   drawTimer = null;
                // })
              ;
            },
            schedule = function() {
              if(redraw && !drawTimer && visible(idx)) {
                drawTimer = $timeout(draw, 0, false);
              }
            },
            discard = function() {
              if(g !== null) {
                $log.log('discarding npactGraph', g.startBase);
                g.destroy();
                g = null;
              }
            };
        $scope.$on(Evt.DRAW, schedule);
        $scope.$on(Evt.REDRAW, function() { redraw = true; schedule();});
        $scope.$on(Evt.REBUILD, function() { discard(); redraw = true; schedule(); });
        $scope.$on('$destroy', discard);
      }
    };
  })
  .directive('npactExtract', function(STATIC_BASE_URL) {
    return {
      restrict: 'A',
      scope: { extract: '=npactExtract'},
      templateUrl: STATIC_BASE_URL + 'js/graphs/extract.html'
    };
  })
;
