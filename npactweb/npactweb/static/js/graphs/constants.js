angular.module('npact')
  .constant('npactConstants', {
    graphStyle: {
      paddingUnit: 5,
      leftPadding: 120,

      profile: {
        height: 80,
        yStops: [100, 80, 60, 40, 20, 0],
        axis: {
          text: {
            fontSize: 11,
            fill: '#444'
          },
          tickLength: 5
        },
        titleFontSize: 20,
        borderColor: '#777',
        shadeColor: '#f2f2f2',
        tickLength: 5
      },
      tracks: {
        text: {
          fontSize: 9,
          fill: '#444'
        },
        arrow: {
          height: 12,
          width: 6
        },
        guidelines: { stroke: '#bbb'}
      }
    },
    //Indexed by phase
    lineColors : ['rgb(255, 0, 0)', 'rgb(0, 255, 0)', 'rgb(0, 0, 255)'],
    colorBlindLineColors : ['rgb(213, 94, 0)', 'rgb(204, 121, 167)', 'rgb(0, 114, 178)'],
    // how much vertical to use for different kinds of tracks
    trackStyle: {
      default: {
        height: 20,
        strokeWidth: 1,
        light: false
      },
      neworfs: {
        strokeWidth: 2
      },
      modified: {
        light: true
      }
    }
  })
  .service('Err',function() {

    var self = this,
        makeError = function(message, name) {
          self[name] = _.partial(Error, message);
        };

    _.map({TrackNotFound: 'track not found',
           ProfileNotFound: 'profile not found'
          },
          makeError);

  })
  .constant('Evt', {
    DRAW: 'draw',
    REDRAW: 'redraw',
    NOOP: 'noop',
    REBUILD: 'rebuild',
    PAN: 'pan',
    ZOOM: 'zoom',
    GRAPH_REDRAW_COMPLETE: 'graph redraw complete'
  })
// these are expected to match strings specified in pynpact
  .constant('Pynpact', {
    NEW_ORFS: 'File_of_new_CDSs',
    HITS: 'File_of_G+C_coding_potential_regions',
    PDF: 'pdf_filename',
    NPROFILE: 'nprofileData',
    CDS: 'File_of_published_accepted_CDSs',
    ACGT_GAMMA_FILES: 'acgt_gamma_output',
    MODIFIED: 'File_of_published_rejected_CDSs',
    TITLE: 'first_page_title',
    HAS_CDS: 'isgbk',
    EMAIL: 'email',
    CONFIGURE_URL: 'configure_url',
    DDNA_FILE: 'ddna'
  })
;
