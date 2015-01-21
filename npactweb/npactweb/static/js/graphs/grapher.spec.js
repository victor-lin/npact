describe('Grapher', function() {
  'use strict';

  beforeEach(module('assets'));
  beforeEach(module('npact', function($provide) {
    $provide.value('$log', console);
    $provide.service('Fetcher', function() { });
    $provide.value('$timeout', window.timeout);
  }));

  var Grapher, $digest, $timeout, element, npactConstants,
      makeGrapher,
      sampleOpts = {
        startBase: 0,
        endBase: 10000,
        offset: 0,
        width: 600,
        axisTitle: 'C+G'
      };

  beforeEach(function() { element = angular.element('<div id="testGraph"></div>'); });
  afterEach(function() { angular.element(element).remove(); });

  beforeEach(inject(function(_Grapher_, $rootScope, _$timeout_, _npactConstants_,
                      GraphingCalculator) {
    Grapher = _Grapher_;
    npactConstants = _npactConstants_;
    $digest = $rootScope.$digest;
    $timeout = _$timeout_;

    makeGrapher = function() {
      expect(Grapher).toEqual(jasmine.any(Function));
      var opts = angular.extend(
        {}, npactConstants.graphSpecDefaults, sampleOpts);
      opts.colors = npactConstants.lineColors;
      opts.m = GraphingCalculator.chart(opts);
      opts.xaxis = GraphingCalculator.xaxis(opts);

      var g = new Grapher(element[0], opts);
      expect(g).toBeDefined();
      expect(g).toEqual(jasmine.any(Grapher));
      return g;
    };
  }));

  beforeEach(inject(function(NProfiler, TrackReader, headerSpecCalc, $templateCache, $q, $log) {
    NProfiler.ddna = $templateCache.get('/js/test-data/sampleDdnaFile.ddna');
    NProfiler.fetching = $q.when(NProfiler.ddna);

    TrackReader.load('Extracts',
                     $templateCache.get('/js/test-data/NC_007760.genes'))
      .then(function() { $log.log("Finished loading extracts"); });
    TrackReader.load('Hits',
                     $templateCache.get('/js/test-data/NC_007760.profiles'))
      .then(function() { $log.log("Finished loading hits"); });
    sampleOpts = angular.extend(sampleOpts,
                                headerSpecCalc([{text: 'Hits', lineType: 'hits'},
                                                {text: 'Extracts', lineType: 'extracts'}]));
  }));

  it('should have an element to work with', function() {
    expect(element).toBeDefined();
  });

  it('should be valid', function(done) {
    var g = makeGrapher();
    expect(g).toBeDefined();
    expect(g.onDragEnd).toEqual(jasmine.any(Function));
    g.redraw(sampleOpts).then(function() {
      done();
    });
  });
});
