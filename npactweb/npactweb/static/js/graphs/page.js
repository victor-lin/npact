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
  .controller('npactGraphPageCtrl', function($scope, $q, $log, $window,
                                      Fetcher, FETCH_URL, EmailBuilder,
                                      GraphConfig, FileManager, kickstarter) {
    'use strict';

    $scope.miscFiles = [];
    $scope.FETCH_URL = FETCH_URL;
    $scope.config = GraphConfig;
    $scope.email = EmailBuilder.send;
    kickstarter.start();
    $scope.$watch(FileManager.getFiles, function(val) {
      $scope.miscFiles = val;
    }, true);


    var _doPrint = function() {
      var t1 = new Date();
      $log.log("print requested: ", t1);
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
      });
    };

    this.print = function() {
       $( "#printConfirm" ).dialog({
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

  })

  .service('kickstarter', function($q, $log, processOnServer, MessageBus,
                            NProfiler, PredictionManager, ExtractManager, FileManager) {
    'use strict';
    //Kickstart the whole process, start all the main managers
    this.start = function() {
      this.basePromise = processOnServer('parse');
      MessageBus.info('kickstarting', this.basePromise);
      this.basePromise.then(NProfiler.start);
      this.basePromise.then(PredictionManager.start);
      this.basePromise.then(ExtractManager.start);
      this.basePromise.then(FileManager.start);
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
      MessageBus.info("Fetching extract data from server", p);
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
          self.toggleTrack(result.newCds, false);
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

    self.newCds = function(result) {
      if(!result.newCds) {
        result.newCds = Fetcher.fetchFile(result.config[Pynpact.NEW_CDS])
          .then(function(data) {
            var name = 'New ORFs @' + result.config.significance,
                track = new Track(name, data, 'extracts', 15 - result.config.significance);
            GraphConfig.loadTrack(track);
            return track;
          });
      }
      self.toggleTrack(result.newCds, true);
    };

    self.onSignificanceChange = function(significance, oldSig) {
      self.disableOld(oldSig);
      if(significance) {
        $log.log("New prediction significance: ", significance);
        if(!results[significance]) {
          results[significance] = processOnServer('acgt_gamma')
            .then(function(config) {
              return StatusPoller.start(config[Pynpact.ACGT_GAMMA_FILES]).
                then(function(path) { return ({ path: path, config: config }); });
          });
        }
        var waitOn = results[significance];
        waitOn.then(self.newHits);
        waitOn.then(self.newCds);
        waitOn.then(self.updateFiles);
        MessageBus.info('Identifying significant 3-base periodicities @ ' + significance, waitOn);
      }
    };
  })
  .service('FileManager', function(PredictionManager, StatusPoller, Pynpact, $log) {
    'use strict';
    var pdffile = null;
    this.start = function(config) {
      StatusPoller.start(config[Pynpact.PDF])
        .then(function(pdfFilename) {
          $log.log('PDF ready', pdfFilename);
          pdffile = pdfFilename;
        });
    };
    this.getFiles = function() {
      var list = [];
      if(pdffile) {
        list.push(pdffile);
      }
      if(PredictionManager.files) {
        list.push.apply(list, PredictionManager.files);
      }
      return list;
    };
  })
;
