<module>
	<service><?=$GETCFG_SVC?></service>	
	<runtime>
		<services>
			<timezone>												
				<?echo dump(2, "/runtime/services/timezone");?>
			</timezone>
		</services>
	</runtime>
	<SETCFG>ignore</SETCFG>
	<FATLADY>ignore</FATLADY>
	<ACTIVATE>ignore</ACTIVATE>			
</module>
