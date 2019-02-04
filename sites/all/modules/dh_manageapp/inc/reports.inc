<?php


function dh_manage_at_schedule()
{
	global $user;
	$centre_id = arg(1);
	drupal_add_css(drupal_get_path( 'module', 'dh_manageapp').'/css/manageapp.css');
	
	$course = db_query("select c_id from dh_course where c_center=$centre_id")->fetchAssoc();
	

	$q = "Select ct_id, c_id, c_start, c_end, c_name, CONCAT(t_f_name, ' ', t_l_name) as 'teacher', ct_type as 'Type', t_gender from dh_course left join dh_course_teacher on ct_course=c_id left join dh_teacher on ct_teacher=t_id where c_start >= ((CURDATE() - INTERVAL 1 YEAR)) and (c_deleted = 0) and c_center=$centre_id order by c_start asc, ct_type desc";

	$result = db_query($q);
	$data = array();

	while($r = $result->fetchAssoc())
	{	
		$data[$r['c_id'].'||||'.$r['c_name'].'||||'.$r['c_start']][$r['t_gender']][] = array('type' => $r['Type'], 'teacher' => $r['teacher']) ;
		
	}
	$old_rows = $upcoming_rows = array();
	$gender = array('M', 'F');
	foreach($data as $course => $course_data)
	{
		unset($row);	
		$t = explode("||||", $course);
		$row[] = $t[1];

		foreach($gender as $g)
		{			
			$temp = "";
			if (isset($course_data[$g]))
			{
				foreach($course_data[$g] as $gender_data )
				{
					$temp .= $gender_data['teacher'].' - '.$gender_data['type'].'<br>' ;
				}
			}
			$row[] = $temp;
		}

		$row[] = l("View", "assign-teacher/$centre_id/".$t[0]);
		if ( $t[2]  <= date('Y-m-d') ) 

			$old_rows[] = $row;
		else
			$upcoming_rows[] = $row;
			
	}
	
	$header = array('Course Name', 'Male', 'Female', 'Action');
	$out = '<h3>'.l("Back to Dashboard", "centre/".arg(1)).'</h3>';


	$completed = theme("table", array('header' => $header, 'rows' => $old_rows ));
	$fieldset_vars = array(
		'element' => array(
		'#title' => t('Completed courses'),
		'#collapsible' => true,
		'#collapsed' => true,
		'#value' => '',
		'#children' => $completed
		),
	);

	$out .= theme('fieldset', $fieldset_vars);


	$out .= '<h3>Upcoming courses</h3>';
	$out .= theme("table", array('header' => $header, 'rows' => $upcoming_rows ));


	return $out;
}






function dh_manageapp_report_form($form, &$form_state)
{
		global $user;
  	$_SESSION['lasturi'] = $_SERVER['REQUEST_URI'];




	$iPod    = stripos($_SERVER['HTTP_USER_AGENT'],"iPod");
	$iPhone  = stripos($_SERVER['HTTP_USER_AGENT'],"iPhone");
	$iPad    = stripos($_SERVER['HTTP_USER_AGENT'],"iPad");
	drupal_add_css( libraries_get_path("editor"). "/DataTables/datatables.min.css");
	drupal_add_js( libraries_get_path("editor").  "/DataTables/datatables.min.js");
	if ( !($iPad || $iPhone || $iPod) )
	{
		//drupal_add_js( libraries_get_path("editor").  "/DataTables/Buttons-1.5.1/js/buttons.html5.min.js");
		drupal_add_js( libraries_get_path("editor").  "/DataTables/Buttons-1.5.1/js/buttons.print.min.js");
	}

	drupal_add_css( drupal_get_path('module', 'dh_manageapp'). "/css/jquery-confirm.min.css");
	drupal_add_js( drupal_get_path('module', 'dh_manageapp'). "/js/jquery-confirm.min.js");
	
	drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/tables.css' );











	global $user;
	$centre_id = '';
	if (arg(1) <> '')
	{
		$centre_id = arg(1);
	}
	if ( isset($form_state['storage']) )
	{

		$query="SELECT td_id,td_val1 from dh_type_detail where td_type='COURSE-TYPE' and td_active=1";

				$courses = db_query($query)->fetchAllKeyed();
					$courses[''] = 'Choose';
				$courses['All'] = 'All';
		$storage = $form_state['storage'];
		// print_r($storage['center_name']);
		// exit();
		$breadcrumb = '';
		if ($centre_id <> '')
		{
			$breadcrumb .= l("Back to Dashboard", "centre/".$centre_id);	
		}
		if ($breadcrumb <> '')
		{
			$form['a'] = array('#markup' => '<h3>'.$breadcrumb.'</h3>');
		}
		$CenterQueryStr ='';
		if (user_access("access all centres"))
		{

			$CenterName= $form_state['storage']['center_name'];
			$CenterQueryStr = 'c_center in ('.implode(",", array_values($CenterName)).')';
			
		 	
		}
		else
		{
			$CenterQueryStr='c_center='.$centre_id;
		}

		if (user_access("access all centres"))
		{
			$center_query="SELECT c_id,c_name from dh_center,dh_user_center where uc_center = ".$user->uid." and uc_user =1 and  uc_deleted=0";

			$center = db_query($center_query)->fetchAllKeyed();
			$form['#attributes'] = array('id' => array('center-dropdown')); 
			// $form['center'] = array('#title' => 'center',  '#type' => 'select', '#options' => $center,'#required' => TRUE,'#attributes' => array('id'=>'center_dropdown'));

			$form['center_name'] = array(
            '#type' => 'select',
	  	    '#multiple' => TRUE,
		    '#title' => t('Center Name'),
		    '#required' => TRUE,
		    '#options' => $center,
		    '#size' => 5,
		    '#attributes' => array('id'=>'center_name'),
            '#weight' => 0);
		}

			$form['#attributes'] = array('class' => array('container-inline')); 
		
			$form['course'] = array('#title' => 'Course', '#type' => 'select', '#options' => $courses,'#required' => TRUE,'#default_value' => 'All');
			$form['from_date'] = array(
				'#type' => 'textfield',
				'#title' => t('Start Date'),
				'#attributes' => array('placeholder' => t('Enter Start Date'),'id'=>'from_date','class' => array('from_date'),'autocomplete' => 'off'),
				'#required' => TRUE,
				'#size' => 40,
			);

			$form['to_date'] = array(
				'#type' => 'textfield',
				'#title' => t('End Date'),
				'#attributes' => array('placeholder' => t('Enter End Date'),'id'=>'to_date','class' => array('to_date','datepicker'),'autocomplete' => 'off'),
				'#required' => TRUE,
				'#size' => 40,
			);
		$InlineJS="(function ($) { $('.datepicker').datepicker({ dateFormat: 'dd-mm-yy' });
				$('.from_date').datepicker({ dateFormat: 'dd-mm-yy',maxDate: '-1d', 
	        		onSelect: function (selectedDate ) {
	            		var dt2 = $('.to_date');
	            	
	            	    dt2.datepicker( 'option', 'maxDate', 0);
	            		}
	            	});
	          
	            })(jQuery);";

			drupal_add_library('system', 'ui.datepicker');

			drupal_add_js($InlineJS, array('type' => 'inline', 'scope' => 'footer', 'weight' => 4));
			$form['sub'] = array('#type' => 'submit', '#value' =>'Go');
		
		
			$form['#submit'][]='dh_manageapp_report_display';
		
			$storage = $form_state['storage'];
			$course=$storage['course'];
 		 	$from_date=$storage['from_date'];
		 	$to_date=$storage['to_date'];
		 	$from_date=date('Y-m-d' , strtotime($from_date));
		 	$to_date=date('Y-m-d' , strtotime($to_date));

		 	$append ='';
		 	if ($course <> 'All')
		 	{
		 	  $append = 'and dh_course.c_course_type='.$course;
		 	}

			 $Query2="SELECT dh_course_teacher.ct_teacher,
							dh_teacher.t_f_name,
							dh_teacher.t_m_name,
							dh_teacher.t_l_name,
							dh_course_teacher.ct_type,
							dh_course_teacher.ct_course,
							dh_teacher.t_gender
					   FROM dh_course_teacher,dh_teacher,dh_course 
					  where dh_teacher.t_id=dh_course_teacher.ct_teacher and 
							dh_course_teacher.ct_course=dh_course.c_id and
	                        ($CenterQueryStr)  
	                        $append 
		                and ct_status='Confirmed'";
				
						$result2 = db_query($Query2);
						$rows = array();
						while ($r2=$result2->fetchAssoc())
	    				{
							$Name=$r2['t_f_name'].' '.$r2['t_l_name'];
							
							$Gender=$r2['t_gender'];
							$Type=$r2['ct_type'];
							$cs_course=$r2['ct_course'];
							$rows[$cs_course]['TeacherData'][]=array('FNAME'=>$Name ,
						                    'Gender'=>$Gender,
						                    'Type'=>$Type);
						} 


	 		// $Query="SELECT dh_course_stat.*,c_center,c_name,c_id 
		 	// 		 FROM dh_course_stat,dh_course 
		 	// 		where cs_course=c_id and 
		 	// 			  ($CenterQueryStr) and 
		 	// 			  c_course_type=$course and cs_status='Attended' and
		 	// 			  c_start between '$from_date' AND '$to_date' 
		 	// 	 order by cs_course,cs_gender ASC";
		 	// if($course == "All")
		 	// {
		 	// 	//remove the course type from where condition
		 	// 	$Query="SELECT dh_course_stat.*,c_center,c_name,c_id 
		 	// 		 FROM dh_course_stat,dh_course 
		 	// 		where cs_course=c_id and 
		 	// 			  ($CenterQueryStr) and  cs_status='Attended' and
		 	// 			  c_start between '$from_date' AND '$to_date' 
		 	// 	 order by cs_course,cs_gender ASC";
		 	// }
				$Query="SELECT dh_course_stat.*,c_center,
		 					   dh_course.c_name,
		 					   dh_course.c_id ,
		 					   dh_center.c_name as 'CenterName'
		 			      FROM dh_course_stat,
		 			      	   dh_course,
		 			      	   dh_center
		 				 where cs_course=dh_course.c_id and 
						  	   dh_course.c_center = dh_center.c_id and
		 				  	   ($CenterQueryStr) and 
		 				  	   c_course_type=$course and 
		 				  	   cs_status='Attended' and
		 				  	   c_start between '$from_date' AND '$to_date' 
		 		 	  order by cs_course,cs_gender ASC";
		 	if($course == "All")
		 	{
		 		//remove the course type from where condition
		 		$Query="SELECT dh_course_stat.*,c_center,
		 					   dh_course.c_name,
		 					   dh_course.c_id ,
		 	                   dh_center.c_name as 'CenterName'
		 			      FROM dh_course_stat,
		 			      	   dh_course,
		 			      	   dh_center
		 				 where cs_course=dh_course.c_id and 
						  	   dh_course.c_center = dh_center.c_id and
		 				  	   ($CenterQueryStr) and  
		 				  	   cs_status='Attended' and
		 				  	   c_start between '$from_date' AND '$to_date' 
		 		      order by cs_course,cs_gender ASC";
		 	}
		 	  $result = db_query($Query);
		 	   
		 
			   $Total = 0;
			   $TeacherArr=$rows2=$CourseArr=$CourseIDArr=array();
			   $fields = array('NM', 'NF', 'OM', 'OF', 'SM', 'SF');
			   	while( $r = $result->fetchAssoc() )
				{
					$cs_course=$r['cs_course'];
					$c_id=$r['c_id'];
					
					$cs_status=$r['cs_status'];
					$cs_gender=$r['cs_gender'];
					$cs_old=$r['cs_old'];
					$cs_count=$r['cs_count'];
					$rows[$cs_course]['c_name']=$r['c_name'];
				
					$field =  ($cs_old?'O':'N').$cs_gender;
					// if ($cs_sevak)
					//	$field[0] = 'S'; 

					$rows[$cs_course][$field] = $cs_count;
					$rows[$cs_course]['SevakMale']=0;
					$rows[$cs_course]['SevakFemale']=0;
					$rows[$cs_course]['Foreigner']=0;
					$rows[$cs_course]['CenterName']=$r['CenterName'];
				}

			
			foreach ($rows as $key => $value) {
						$ArrCount=count($value['TeacherData']);
						$value['TeacherCount']=$ArrCount;
						$CenterName=str_replace(' ', '_', $value['CenterName']);
						// $TempArr[]=$value;
						$TempArr[$CenterName][]=$value;
			}
		
			foreach ($TempArr as $key => $value) {
				
					$header =$r= array();
					$attributes = array('id' => $key);
					$form['myfieldset'][$key] = array(
					  '#theme' => 'table',
					  '#attributes' => $attributes,
					  '#header' => $header,
					  '#rows' => $r
					);
			}
		
		$data = json_encode($TempArr);


		$button_access = '';
		if (user_access('export data'))
			if ( !($iPad || $iPhone || $iPod) )
				$button_access = '{extend: "excel",
            					   title: "Statistics Data",
            	}';

	


		$js = '
			function format ( d ) {
				 var $Design = "<div class=\"slider\"><table cellpadding=\"5\" cellspacing=\"0\" border=\"0\" style=\"padding-left:50px;width:100%;\">";
				 $Design +="<thead><tr><td><b>Name</b></td><td  align=\"center\"><b>Gender<b></td><td><b>Type</b></td></tr></thead>";	        
					jQuery.each(d.TeacherData, function (i, object)
					{
							
					            $Design +="<tr><td>"+object.FNAME+"</td><td  align=\"center\">"+object.Gender+"</td><td>"+object.Type+"</td></tr>";
					});
			        $Design +="</table></div>";
					return  $Design;
			
			}

			(function ($) {
				$(document).ready(function(){

					var dataset = '.$data.';
					var comment = "";			
			
					
				    var iOS = !!navigator.platform && /iPad|iPhone|iPod/.test(navigator.platform);
					$.each(dataset, function (key, object)
					{					
							
						   var TableIDWithHash="#"+key; 
						   var TableWrapper="#"+key+"_wrapper"; 
						   var CenterName=object[0][\'CenterName\']; 
						   $(TableIDWithHash).append("<caption style=\'caption-side: top\' class=\'StatisticsTableCaption\'>"+CenterName+"</caption>");
						   
  						   $(".table-responsive").css({"margin-bottom":"25px"});
  						   $(".StatisticsTableCaption").css({"color":"black","font-weight":"bold"});
						    var buttonCommon = {
						        exportOptions: {
						            format: {
						            	header: function( object, column, node ) {
						            		var h = ["FName", "Gender", "Type"]; 
						            		return h[column];
						            	},
						                body: function ( object, row, column, node ) {
						                	var tmp = table.rows().data();
						                	var a;
						                	
						                	return object;
						                }
						            }
						        }
						    };


							var table = $(TableIDWithHash).DataTable({
								data: object,					
						        columns: [	
						        	{
						                "class":          "details-control",
						                "orderable":      false,
						                "data":           null,
						                "title":		  "Detail",
						                "defaultContent": ""
						            },		        
						            { data: "c_name", "title" : "Name" },
						            { data: "NM", "title" : "New Male" },
							        { data: "OM", "title" : "Old Male"},
						   			{ data: "NF", "title" : "New Female" },
							        { data: "OF", "title" : "Old Female" },
							        { data: "TeacherCount", "title" : "Teacher" },
							        { data: "SevakMale", "title" : "Sevak Male" },
							        { data: "SevakFemale", "title" : "Sevak Female" }, 
							        { data: "Foreigner", "title" : "Foreigner" }
					            ], 
					            dom: "lBfrtip",
					            buttons: ['.$button_access.'
            						
        						]  	
							});

							$(TableIDWithHash+" tbody").on("click", "td.details-control", function () {
					        var tr = $(this).closest("tr");
					        var row = table.row( tr );
					 
					        if ( row.child.isShown() ) {
					            // This row is already open - close it
					            $("div.slider", row.child()).slideUp( function () {
					                row.child.hide();
					                tr.removeClass("shown");
					            });
					        }
					        else {
					            // Open this row
					            row.child( format(row.data()), "no-padding" ).show();
					            tr.addClass("shown");
					 
					            $("div.slider", row.child()).slideDown();		            	
					        }
					    });
					    	
					});
					
				});
			})(jQuery);
		';

		drupal_add_js($js, 'inline');	
		return $form;
	}
	else
	{
		$query="SELECT td_id,td_val1 from dh_type_detail where td_type='COURSE-TYPE' and td_active=1";

		$courses = db_query($query)->fetchAllKeyed();
			$courses[''] = 'Choose';
		$courses['All'] = 'All';
		drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/select2.min.css');
		drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/manageapp.css');
		drupal_add_js(drupal_get_path('module', 'dh_manageapp').'/js/select2.min.js');

		

	
		$breadcrumb = '';
		if ($centre_id <> '')
		{
			$breadcrumb .= l("Back to Dashboard", "centre/".$centre_id);	
		}
		if ($breadcrumb <> '')
		{
			$form['a'] = array('#markup' => '<h3>'.$breadcrumb.'</h3>');
		}

		if (user_access("access all centres"))
		{
			$center_query="SELECT c_id,c_name from dh_center,dh_user_center where uc_center = ".$user->uid." and uc_user =1 and  uc_deleted=0";

			$center = db_query($center_query)->fetchAllKeyed();
			$form['#attributes'] = array('id' => array('center-dropdown')); 
			// $form['center'] = array('#title' => 'center',  '#type' => 'select', '#options' => $center,'#required' => TRUE,'#attributes' => array('id'=>'center_dropdown'));

			$form['center_name'] = array(
            '#type' => 'select',
	  	    '#multiple' => TRUE,
		    '#title' => t('Center Name'),
		    '#required' => TRUE,
		    '#options' => $center,
		    '#size' => 5,
		    '#attributes' => array('id'=>'center_name'),
            '#weight' => 0);
		}
			$form['#attributes'] = array('class' => array('container-inline')); 
		
			$form['course'] = array('#title' => 'Course', '#type' => 'select', '#options' => $courses,'#required' => TRUE,'#default_value' => 'All');
			$form['from_date'] = array(
				'#type' => 'textfield',
				'#title' => t('Start Date'),
				'#attributes' => array('placeholder' => t('Enter Start Date'),'id'=>'from_date','class' => array('from_date'),'autocomplete' => 'off'),
				'#required' => TRUE,
				'#size' => 40,
			);

			$form['to_date'] = array(
				'#type' => 'textfield',
				'#title' => t('End Date'),
				'#attributes' => array('placeholder' => t('Enter End Date'),'id'=>'to_date','class' => array('to_date','datepicker'),'autocomplete' => 'off'),
				'#required' => TRUE,
				'#size' => 40,
			);
		$InlineJS="(function ($) { $('.datepicker').datepicker({ dateFormat: 'dd-mm-yy' });
				$('.from_date').datepicker({ dateFormat: 'dd-mm-yy',maxDate: '-1d', 
	        		onSelect: function (selectedDate ) {
	            		var dt2 = $('.to_date');
	            	
	            	    dt2.datepicker( 'option', 'maxDate', 0);
	            		}
	            	});
	    /*     
		$(document).ready(function(){
			$('#center_dropdown').select2();
		});
		*/
		})(jQuery);";

			drupal_add_library('system', 'ui.datepicker');

			drupal_add_js($InlineJS, array('type' => 'inline', 'scope' => 'footer', 'weight' => 4));
			$form['sub'] = array('#type' => 'submit', '#value' =>'Go');
			$form['#submit'][]='dh_manageapp_report_display';
			return $form;
	}
}
function dh_manageapp_report_display($form, &$form_state)
{
	$form_state['storage'] = $form_state['input'];
	$form_state['rebuild'] = TRUE;
}

?>