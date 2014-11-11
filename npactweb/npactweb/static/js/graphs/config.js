angular.module('npact')
  .service('GraphConfig', function(Err, npactConstants, Evt, $log) {
    var self = this;
    self.tracks = [];
    self.colorBlindFriendly = false;
    self.basesPerGraph = 10000;
    self.width = null;
    self.profileSummary = null;
    self.offset = 0; // how much the graph is panned left/right


    var activeTracks = function(){
      return _.filter(self.tracks, {active: true}, 'active');
    };

    /**
     * do we have a track with a given name?
     */
    this.hasTrack = function(name){
      return _.some(self.tracks, {text: name});
    };

    /**
     * register a track to be displayed on the graph
     */
    this.loadTrack = function(name, type) {
      if(self.hasTrack(name)){ throw new Err.TrackAlreadyDefined(); }
      $log.log('loading track', name, type);
      self.tracks.push({
        text: name,
        lineType: type,
        active: true
      });
    };

    /**
     * calcuate the header information
     */
    this.headerSpec = function() {
      var offset = npactConstants.graphSpecDefaults.headerY;
      var headers = _.map(activeTracks(), function(cfg) {
        var h = npactConstants.headerSizes[cfg.lineType],
            y = offset;
        offset += h;
        return {
          text: cfg.text,
          lineType: cfg.lineType,
          y: y,
          height: h
        };
      });
      return {
        headers: headers,
        headerY: offset
      };

    };


    /**
     * analyze a previous version of the config, and return what command
     * to run in order to refresh the page
     */
    this.refreshCommand = function(oldGraphConfig) {
      var noBasesPerGraph = _.isUndefined(self.basesPerGraph),
          sameBasesPerGraph = self.basesPerGraph === oldGraphConfig.basesPerGraph,
          sameOffset = self.offset === oldGraphConfig.offset,
          hasProfile = self.profileSummary !== null,
          hadProfile = oldGraphConfig.profileSummary !== null,
          newProfile = hasProfile && !hadProfile;

      // happens with invalid input in config box
      if(noBasesPerGraph) { return Evt.NOOP;}
      if(!sameBasesPerGraph || newProfile || !sameOffset){ return Evt.REBUILD; }
      if(hasProfile){ return Evt.REDRAW; }
      return Evt.NOOP;
    };
  })
  .directive('npactGraphConfig', function npactGraphConfig(STATIC_BASE_URL, GraphConfig) {
    return {
      restrict: 'A',
      templateUrl:STATIC_BASE_URL+'js/graphs/config.html',
      link: function($scope){
        $scope.gc = GraphConfig;
      }
    };
  });
