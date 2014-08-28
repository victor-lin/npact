angular.module('npact')
  .directive('npactGraphPage', function(STATIC_BASE_URL, Utils, GraphDealer) {
    return {
      restrict: 'A',
      templateUrl: STATIC_BASE_URL + 'js/graphs/page.html',
      controller: 'npactGraphPageCtrl',
      link: function($scope, $element, $attrs) {
	// TODO: watch for changes in width
	// http://stackoverflow.com/questions/23044338/window-resize-directive
        Utils.widthAvailable($element).then(GraphDealer.setWidth);
      }
    };
  })

  .controller('npactGraphPageCtrl', function($scope, KICKSTART_BASE_URL, GraphDealer, $window, Fetcher, npactConstants) {

    $scope.miscFiles = [];
    $scope.graphHeight = npactConstants.graphSpecDefaults.height;

    Fetcher.rawFile(KICKSTART_BASE_URL + $window.location.search)
      .then(function(config) {
        $scope.title = config.first_page_title;
        Fetcher.nprofile(config).then(GraphDealer.setProfile);
        Fetcher.inputFileCds(config)
          .then(function(data) {
            GraphDealer.addExtract({name: 'Input file CDS', data: data});
          });

        Fetcher.acgtGammaFileList(config).then(function(fileList) {
          $scope.miscFiles.push.apply($scope.miscFiles, fileList);
          Fetcher.fetchFile(config['File_of_new_CDSs'])
            .then(function(data) {
              GraphDealer.addExtract({name: 'Newly Identified ORFs', data: data});
            });

          //TODO: Hits line: config['File_of_G+C_coding_potential_regions']
          // Fetcher.fetchFile(config['File_of_G+C_coding_potential_regions'])
          //   .then(hitsParser)
          //   .then(function(data) {
          //     //TODO: GraphDealer.addHits
          //     //GraphDealer.addHits({name: 'Hits', data: data});
          //   });
        });
      });
  })

  .service('Fetcher', function(StatusPoller, $http, FETCH_URL, ACGT_GAMMA_FILE_LIST_URL) {

    /**
     * download contents from any url
     */
    function rawFile(url) {
      return $http.get(url).then(function(res) { return res.data; });
    };

    /**
     * download contents from a "fetch" path
     */
    function fetchFile(path){
      return rawFile(FETCH_URL + path);
    }

    this.rawFile = rawFile;
    this.fetchFile = fetchFile;

    this.nprofile = function(config) {
      return StatusPoller.start(config['nprofileData'])
        .then(fetchFile);
    };
    this.inputFileCds = function(config) {
      return StatusPoller.start(config['Input file CDS'])
        .then(fetchFile);
    };

    this.acgtGammaFileList = function(config) {
      return StatusPoller.start(config['acgt_gamma_output'])
        .then(function(path) {
          return rawFile(ACGT_GAMMA_FILE_LIST_URL + path);
        });
    };
  })


  .service('StatusPoller', function(STATUS_BASE_URL, $q, $http, $timeout) {
    var POLLTIME = 2500;

    function poller(tid, deferred) {
      // remember our arguments
      var pollAgain = _.partial(poller, tid, deferred);

      $http.get(STATUS_BASE_URL + tid)
        .then(function(res) {
          if(res.ready) { deferred.resolve(tid); }
          else { $timeout(pollAgain, POLLTIME); }
        })
        .catch(deferred.reject);

      return deferred.promise;
    }

    this.start = function(tid) {
      if(!tid || !tid.length == 0)
	return $q.reject(new Error("Invalid task id"));

      return poller(tid, $q.defer());
    };
  });
