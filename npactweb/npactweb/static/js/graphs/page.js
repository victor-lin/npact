angular.module('npact')
  .directive('npactGraphPage', function(STATIC_BASE_URL) {
    'use strict';
    return {
      restrict: 'A',
      templateUrl: STATIC_BASE_URL + 'js/graphs/page.html',
      controller: 'npactGraphPageCtrl',
      controllerAs: 'pageCtrl'
    };
  })

  .controller('npactGraphPageCtrl', function($scope, $q, $log, $window, dialogService,
                                      Fetcher, FETCH_URL, EmailBuilder, STATIC_BASE_URL,
                                      GraphConfig,  kickstarter, processOnServer) {
    'use strict';

    $scope.FETCH_URL = FETCH_URL;
    $scope.config = GraphConfig;
    $scope.email = EmailBuilder.send;
    kickstarter.start();


    var _doPrint = function() {
      var t1 = new Date();
      $log.log("print requested: ", t1);
      $scope.$broadcast('printresize', true);
      $scope.printCounter = 0;
      var waitList = [];
      var evt = $scope.$broadcast('print', function(promise) {
        $scope.printCounter++;
        waitList.push(promise);
      });
      $q.all(waitList).then(function() {
        $scope.printCounter = 0;
        $log.log('Finished rendering', new Date() - t1);
        $window.print();
        $scope.$broadcast('printresize', false);
      });
    };

    this.print = function() {
      var printTemplate = STATIC_BASE_URL + 'js/graphs/printConfirm.html';
      dialogService.open('printConfirm', printTemplate, null, {
         resizable: false,
         modal: true,
         buttons: {
           "Cancel": function() {
             $( this ).dialog( "close" );
           },
           "Proceed": function() {
             $( this ).dialog( "close" );
             _doPrint();
           }
         }
       });
    };
    this.requestPDF = function() {
      processOnServer('allplots').catch(function(e) {
        $log.error('Error requesting PDF:', e);
      });
    };
  })

  .controller('DownloadsCtrl', function($scope, $log, PredictionManager, MessageBus, Pynpact, StatusPoller, GraphConfig, STATIC_BASE_URL, dialogService) {
    'use strict';
    $scope.$watch( function() { return PredictionManager.files; },
                   function(val) { $scope.predictionFiles = val; });
    $scope.$watch(
      function() { return GraphConfig[Pynpact.PDF]; },
      function(pdfFilename) {
        if(!pdfFilename) return;
        var p = StatusPoller.start(pdfFilename)
          .then(function(pdfFilename) {
            $log.log('PDF ready', pdfFilename);
            var dialogTemplate = STATIC_BASE_URL + 'js/graphs/pdfReady.html';
            dialogService.open('pdfReady', dialogTemplate, { pdf: pdfFilename });
            $scope.pdf = pdfFilename;
          });
        MessageBus.info("Generating PDF", p);
      });
  })

  .service('kickstarter', function($q, $log, processOnServer, MessageBus,
                            NProfiler, PredictionManager, ExtractManager) {
    'use strict';
    //Kickstart the whole process, start all the main managers
    this.start = function() {
      this.basePromise = processOnServer('parse');
      var safePromise = this.basePromise.catch(function(e) {
        $log.error(e);
        MessageBus.danger('There\'s been an error starting up; please try starting over.');
      });
      MessageBus.info('kickstarting', safePromise);
      this.basePromise.then(NProfiler.start);
      this.basePromise.then(PredictionManager.start);
      this.basePromise.then(ExtractManager.start);
      return this.basePromise;
    };
  })

  .service('ExtractManager', function(Fetcher, Pynpact, Track, GraphConfig,
                               processOnServer, MessageBus, $log) {
    'use strict';
    this.start = function(config) {
      if(config.format != 'genbank') { return; }
      var p = processOnServer('extract').then(function(config) {
        if(config[Pynpact.CDS]) {
          Fetcher.pollThenFetch(config[Pynpact.CDS])
            .then(function(data) {
              return GraphConfig.loadTrack(new Track('Input file CDS', data, 'extracts'));
            });
        }
      });
      MessageBus.info(
        "Fetching extract data from server",
        p.catch("Failure while extracting known genes."));
    };
  })
  .service('PredictionManager', function(Fetcher, StatusPoller, Pynpact, Track,
                                  GraphConfig, processOnServer, $log, MessageBus,
                                  ACGT_GAMMA_FILE_LIST_URL) {
    'use strict';
    var self = this;
    self.files = null;
    var results = {}; //hash keyed on significance of already requested results.

    self.updateFiles = function(result) {
      var path = result.path;
      if(!result.files) {
        $log.log('Fetching the ACGT_GAMMA_FILE_LIST from', path);
        result.files = Fetcher.rawFile(ACGT_GAMMA_FILE_LIST_URL + path);
      }
      result.files.then(function(files) { self.files = files; });
    };
    self.toggleTrack = function(ptrack, active) {
      if(ptrack) { ptrack.then(function(track) { track.active = active; }); }
    };
    self.disableOld = function(oldSig) {
      if(oldSig && results[oldSig]) {
        results[oldSig].then(function(result) {
          self.toggleTrack(result.hits, false);
          self.toggleTrack(result.newORFs, false);
          self.toggleTrack(result.modified, false);
          self.files = null;
        });
      }
    };
    self.newHits = function(result) {
      if(!result.hits) {
        result.hits = Fetcher.fetchFile(result.config[Pynpact.HITS])
          .then(function(data) {
            var name = 'Hits @' + result.config.significance,
                track = new Track(name, data, 'hits', 100 - result.config.significance);
            GraphConfig.loadTrack(track);
            return track;
          });
      }
      self.toggleTrack(result.hits, true);
    };

    self.newORFs = function(result) {
      if(!result.newORFs) {
        result.newORFs = Fetcher.fetchFile(result.config[Pynpact.NEW_ORFS])
          .then(function(data) {
            var name = 'New ORFs @' + result.config.significance,
                track = new Track(name, data, 'neworfs', 15 - result.config.significance);
            GraphConfig.loadTrack(track);
            return track;
          });
      }
      self.toggleTrack(result.newORFs, true);
    };
    self.newModified = function(result) {
      var config = result.config;
      if(!result.modified) {
        result.modified = Fetcher.fetchFile(config[Pynpact.MODIFIED])
          .then(function(data) {
            var name = 'Modified ORFs @' + config.significance,
                track = new Track(name, data, 'modified', 10 - config.significance);
            GraphConfig.loadTrack(track);
            return track;
          });
      }
      self.toggleTrack(result.modified, true);
    };

    self.onSignificanceChange = function(significance, oldSig) {
      self.disableOld(oldSig);
      if(significance) {
        $log.log("New prediction significance: ", significance);
        if(!results[significance]) {
          results[significance] = processOnServer('acgt_gamma')
            .then(function(config) {
              return StatusPoller.start(config[Pynpact.ACGT_GAMMA_FILES])
                .then(function(path) { return ({ path: path, config: config }); });
          });
        }
        var waitOn = results[significance];
        waitOn.then(self.newHits);
        waitOn.then(self.newORFs);
        waitOn.then(self.newModified);
        waitOn.then(self.updateFiles);
        MessageBus.info(
          'Identifying significant 3-base periodicities @ ' + significance,
          waitOn.catch(function(e) {
            MessageBus.danger('Failure while identifying significant 3-base periodicities');
          }));
      }
    };
  })

  .directive('jqAccordion', function($log) {
    'use strict';
    return {
      scope: true,
      restrict: 'A',
      link: function($scope, $element, $attrs) {
        var defaults = {
          heightStyle: "content",
          collapsible: true,
          active: 0
        };
        var opts = _.transform(defaults, function(acc, v, k, o) {
          v = $attrs[k];
          if(v) {
            acc[k] = $scope.$eval(v);
          }
        }, defaults);
        $($element).accordion(opts);
      }
    };
  })
;
